#ifndef LIBC_TIME_H
#define LIBC_TIME_H

#include "syscall.h"

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

static inline void tzset()
{
    // TODO
}

static inline time_t time(time_t* tloc)
{
    time_t current_time = (time_t)syscall_get_time_up();
    if (tloc)
    {
        *tloc = current_time;
    }
    return current_time;
}

static inline time_t mktime(struct tm* tm)
{
    time_t t = 0;
    t += tm->tm_sec;
    t += tm->tm_min * 60;
    t += tm->tm_hour * 3600;
    t += (tm->tm_mday - 1) * 86400;
    t += tm->tm_mon * 30 * 86400;
    t += (tm->tm_year - 70) * 365 * 86400;
    return t;
}

static inline struct tm *gmtime(const time_t *timep)
{
    static struct tm tm_result;
    time_t t = *timep;
    tm_result.tm_sec = t % 60;
    t /= 60;
    tm_result.tm_min = t % 60;
    t /= 60;
    tm_result.tm_hour = t % 24;
    t /= 24;
    tm_result.tm_mday = (t % 30) + 1;
    tm_result.tm_mon = (t / 30) % 12;
    tm_result.tm_year = 70 + (t / 365);
    tm_result.tm_wday = (t + 4) % 7;
    tm_result.tm_yday = t % 365;
    tm_result.tm_isdst = 0;
    return &tm_result;
}

#endif // LIBC_TIME_H