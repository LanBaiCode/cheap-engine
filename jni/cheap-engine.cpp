#include "cheap-engine.h"
#include "speedhack.h"

ORIG_NANOSLEEP orig_nanosleep = nullptr;
ORIG_CLOCK_GETTIME orig_clock_gettime = nullptr;
CRASH crash_process = nullptr;

#define CHEAPENGINEVERSION 1
char versionstring[] = "CHEAPENGINE Network 1.0";

ssize_t recvall(int s, void *buf, size_t size, int flags)
{
    ssize_t totalreceived = 0;
    ssize_t sizeleft = size;
    unsigned char *buffer = (unsigned char *)buf;

    // enter recvall
    flags = flags | MSG_WAITALL;

    while (sizeleft > 0)
    {
        ssize_t i = recv(s, &buffer[totalreceived], sizeleft, flags);

        if (i == 0)
        {
            LOGD("recv returned 0\n");
            return i;
        }

        if (i == -1)
        {
            LOGD("recv returned -1\n");
            if (errno == EINTR)
            {
                LOGD("errno = EINTR\n");
                i = 0;
            }
            else
            {
                LOGD("Error during recvall: %d. errno=%d\n", (int)i, errno);
                return i; // read error, or disconnected
            }
        }

        totalreceived += i;
        sizeleft -= i;
    }

    // leave recvall
    return totalreceived;
}

ssize_t sendall(int s, void *buf, size_t size, int flags)
{
    ssize_t totalsent = 0;
    ssize_t sizeleft = size;
    unsigned char *buffer = (unsigned char *)buf;

    while (sizeleft > 0)
    {
        ssize_t i = send(s, &buffer[totalsent], sizeleft, flags);

        if (i == 0)
        {
            return i;
        }

        if (i == -1)
        {
            if (errno == EINTR)
                i = 0;
            else
            {
                LOGD("Error during sendall: %d. errno=%d\n", (int)i, errno);
                return i;
            }
        }

        totalsent += i;
        sizeleft -= i;
    }

    return totalsent;
}

int init()
{
#ifdef PAGE_SIZE
    size_t psize = PAGE_SIZE;
#else
    size_t psize = getpagesize();
#endif

    /*
    mov x0,0
    mov lr,0
    br x0
    */
    char *crash_shellcode = const_cast<char *>("\x00\x00\x80\xd2\x1e\x00\x80\xd2\x00\x00\x1f\xd6");
    void *crash_alloc = mmap(nullptr, psize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    memcpy(crash_alloc, crash_shellcode, 12);
    crash_process = reinterpret_cast<CRASH>(crash_alloc);

    /*
    stp     x29, x30, [sp, #-16]!
    mov     x29,  sp
    str     x8,       [sp, #-16]!
    mov     x8, x2
    svc     #0
    ldr     x8,       [sp], #16
    ldp     x29, x30, [sp], #16
    ret
    */
    char *syscall2_shellcode = const_cast<char *>("\xfd\x7b\xbf\xa9\xfd\x03\x00\x91\xe8\x0f\x1f\xf8\xe8\x03\x02\xaa\x01\x00\x00\xd4\xe8\x07\x41\xf8\xfd\x7b\xc1\xa8\xc0\x03\x5f\xd6");
    void *syscall2_alloc = mmap(nullptr, psize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    memcpy(syscall2_alloc, syscall2_shellcode, 32);
    orig_clock_gettime = reinterpret_cast<ORIG_CLOCK_GETTIME>(syscall2_alloc);
    orig_nanosleep = reinterpret_cast<ORIG_NANOSLEEP>(syscall2_alloc);

    return 0;
}

int main_loop()
{
    struct timespec req = {0, 500 * MILLI_SEC};
    while (true)
    {
        speedhack_detect();
        orig_nanosleep(&req, nullptr, __NR_nanosleep);
    }
}

int DispatchCommand(int currentsocket, unsigned char command)
{
    int r;

    // printf("command:%d\n",command);
    switch (command)
    {
    case CMD_GETVERSION:
    {
        PCeVersion v;
        int versionsize = strlen(versionstring);
        v = (PCeVersion)malloc(sizeof(CeVersion) + versionsize);
        v->stringsize = versionsize;
        v->version = CHEAPENGINEVERSION;

        memcpy((char *)v + sizeof(CeVersion), versionstring, versionsize);

        // version request
        sendall(currentsocket, v, sizeof(CeVersion) + versionsize, 0);

        free(v);
        break;
    }
    }
    return 0;
}

int newconnection(int currentsocket)
{
    int r;
    unsigned char command;
    while (true)
    {
        r = recvall(currentsocket, &command, 1, MSG_WAITALL);
        if (r == 1)
        {
            DispatchCommand(currentsocket, command);
        }
        else if (r == 0)
        {
            LOGD("Peer has disconnected\n");
            close(currentsocket);
            return 0;
        }
        else if (r == -1)
        {
            LOGD("read error on socket\n");
            close(currentsocket);
            return 0;
        }
    }

    return 0;
}

int server_thread()
{
    int sock0;
    struct sockaddr_un address;
    socklen_t len;
    int sock;

    LOGD("cheap-engine Server. Waiting for client connection\n");
    sock0 = socket(AF_UNIX, SOCK_STREAM, 0);
    char name[256];
    sprintf(name, " cheap-engine%d", getpid());

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, name);

    int al = SUN_LEN(&address);

    address.sun_path[0] = 0;

    int yes = 1;
    setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
    bind(sock0, (struct sockaddr *)&address, sizeof(address));

    int l = listen(sock0, 32);
    if (l == 0)
        LOGD("Listening success\n");
    else
        LOGD("listen=%d (error)\n", l);
    while (true)
    {
        sock = accept(sock0, (struct sockaddr *)NULL, NULL);
        LOGD("accept=%d\n", sock);

        fflush(stdout);
        std::thread th1(newconnection, sock);
        th1.detach();
    }
    return 0;
}
__attribute__((constructor)) int main_thread()
{
    init();

    std::thread th1(main_loop);
    th1.detach();

    std::thread th2(server_thread);
    th2.detach();

    return 0;
}
