#ifndef LIBC_TIME_H
#define LIBC_TIME_H

#include "syscall.h"

struct timespec {
    time_t tv_sec;  /* секунды */
    long   tv_nsec; /* наносекунды */
};

struct tm {
    int tm_sec;    /* секунды после минуты - [0, 60] */
    int tm_min;    /* минуты после часа - [0, 59] */
    int tm_hour;   /* часы с полуночи - [0, 23] */
    int tm_mday;   /* день месяца - [1, 31] */
    int tm_mon;    /* месяцы с января - [0, 11] */
    int tm_year;   /* годы с 1900 */
    int tm_wday;   /* дни недели с воскресенья - [0, 6] */
    int tm_yday;   /* дни года - [0, 365] */
    int tm_isdst;  /* признак летнего времени */
};

static inline void tzset()
{
    // Заглушка
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
    // Простейшая реализация, не учитывающая високосные годы и т.д.
    time_t t = 0;
    t += tm->tm_sec;
    t += tm->tm_min * 60;
    t += tm->tm_hour * 3600;
    t += (tm->tm_mday - 1) * 86400;
    t += tm->tm_mon * 30 * 86400; // грубая оценка
    t += (tm->tm_year - 70) * 365 * 86400; // грубая оценка
    return t;
}

static inline struct tm *gmtime(const time_t *timep)
{
    static struct tm tm_result;
    // Простейшая реализация, не учитывающая високосные годы и т.д.
    time_t t = *timep;
    tm_result.tm_sec = t % 60;
    t /= 60;
    tm_result.tm_min = t % 60;
    t /= 60;
    tm_result.tm_hour = t % 24;
    t /= 24;
    tm_result.tm_mday = (t % 30) + 1; // грубая оценка
    tm_result.tm_mon = (t / 30) % 12; // грубая оценка
    tm_result.tm_year = 70 + (t / 365); // грубая оценка
    tm_result.tm_wday = (t + 4) % 7; // 1 января 1970 был четверг
    tm_result.tm_yday = t % 365; // грубая оценка
    tm_result.tm_isdst = 0; // не учитываем летнее время
    return &tm_result;
}

#endif // LIBC_TIME_H