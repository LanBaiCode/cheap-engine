#pragma once
#include <iostream>
#include <thread>
#include <time.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <android/log.h>

#include <zlib.h>

#include <errno.h>
#include <signal.h>

#define MILLI_SEC 1000000

#define LOG_TAG "4ntich3at"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)

typedef int (*ORIG_CLOCK_GETTIME)(clockid_t clk_id, struct timespec *tp, int sn);
typedef int (*ORIG_GETTIMEOFDAY)(struct timeval *tv, struct timezone *tz, int sn);
typedef int (*ORIG_NANOSLEEP)(const struct timespec *req, struct timespec *rem, int sn);
typedef void (*CRASH)();

extern ORIG_NANOSLEEP orig_nanosleep;
extern ORIG_CLOCK_GETTIME orig_clock_gettime;
extern CRASH crash_process;

#define CMD_GETVERSION 0

#pragma pack(1)
typedef struct
{
    int version;
    unsigned char stringsize;
    // append the versionstring
} CeVersion, *PCeVersion;
#pragma pack()