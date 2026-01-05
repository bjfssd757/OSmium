#include "rtc.h"
#include "../../portio/portio.h"

static void rtc_wait_ready(void)
{
    while (1)
    {
        outb(0x70, 0x0A);
        if (!(inb(0x71) & 0x80))
            break;
    }
}

static uint8_t cmos_read(uint8_t reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t bcd)
{
    return (bcd & 0x0F) + (bcd >> 4) * 10;
}

void read_rtc_time(uint32_t *hour, uint32_t *minute, uint32_t *second)
{
    rtc_wait_ready();

    uint8_t ss = cmos_read(0x00);
    uint8_t mm = cmos_read(0x02);
    uint8_t hh = cmos_read(0x04);
    uint8_t regB = cmos_read(0x0B);

    uint8_t is_pm = hh & 0x80;

    if (!(regB & 0x04))
    {
        ss = bcd_to_bin(ss);
        mm = bcd_to_bin(mm);
        hh = bcd_to_bin(hh & 0x7F);
    }
    else
    {
        hh &= 0x7F;
    }

    if (!(regB & 0x02) && is_pm)
    {
        hh = (hh + 12) % 24;
    }

    hh = (uint8_t)((hh + TIMEZONE_OFFSET + 24) % 24);

    *hour = hh;
    *minute = mm;
    *second = ss;
}