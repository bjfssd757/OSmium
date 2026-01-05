// clock.h

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

typedef struct
{
    uint8_t hh;
    uint8_t mm;
    uint8_t ss;
} ClockTime;

extern volatile ClockTime system_clock;

void clock_tick();
void format_clock(char *buffer, ClockTime t);
void init_system_clock(void);

#endif