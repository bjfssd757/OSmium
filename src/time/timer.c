#include "../portio/portio.h"
#include "timer.h"
#include "../pic.h"
#include "clock/clock.h"
#include "../multitask/multitask.h"
#include "../graphics/graphics.h"
#include <stdint.h>

#define PIT_CMD_PORT 0x43
#define PIT_COUNTER0 0x40
#define PIT_CMD_VALUE 0x36

volatile uint16_t tick_time = 0;
volatile uint64_t last_ms = 0;
volatile uint32_t seconds = 0;
volatile uint32_t current_ms = 0;
volatile bool screen_refresh_status = true;

uint32_t get_millis(void)
{
    return current_ms;
}

void timer_tick(void)
{
    tick_time++;
    current_ms++;
    if (tick_time >= 1000)
    {
        tick_time = 0;
        seconds++;
        clock_tick();
    }

    /* Update frame every 16 ms */
    if ((tick_time % 16) == 0)
    {
        screen_refresh_status = true;
    }

    pic_send_eoi(0);
}

void init_timer(uint32_t frequency)
{
    if (frequency == 0 || frequency > PIT_FREQUENCY)
    {
        return;
    }

    uint32_t divisor = PIT_FREQUENCY / frequency;

    if (divisor > 0xFFFF)
    {
        divisor = 0xFFFF;
    }

    outb(PIT_CMD_PORT, PIT_CMD_VALUE);                    // Command port
    outb(PIT_COUNTER0, (uint8_t)(divisor & 0xFF));        // Low byte
    outb(PIT_COUNTER0, (uint8_t)((divisor >> 8) & 0xFF)); // High byte
}