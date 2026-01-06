#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

#define PIT_FREQUENCY 1193180U

extern volatile bool screen_refresh_status;

void init_timer(uint32_t frequency);
uint32_t get_millis(void);

#endif