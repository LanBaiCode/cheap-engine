#include "speedhack.h"

void speedhack_detect()
{
    struct timespec ts, ts2;
    struct tm tm;

    orig_clock_gettime(CLOCK_REALTIME, &ts, __NR_clock_gettime);
    clock_gettime(CLOCK_REALTIME, &ts2);
    if (ts.tv_sec != ts2.tv_sec)
    {
        LOGD("speedhack detect!!");
        crash_process();
    }
}