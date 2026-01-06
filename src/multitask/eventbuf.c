#include "eventbuf.h"
#include <stddef.h>

event_t event_buf[MAX_EVENT_BUF];
int event_buf_head = 0, event_buf_tail = 0, event_buf_count = 0;

void k_put_event(const event_t *e)
{
    if (event_buf_count >= MAX_EVENT_BUF) return;
    event_buf[event_buf_tail] = *e;
    event_buf_tail = (event_buf_tail + 1) % MAX_EVENT_BUF;
    event_buf_count++;
}

int sys_get_events(event_t *dst, size_t max_count)
{
    size_t out = 0;
    while (event_buf_count > 0 && out < max_count)
    {
        dst[out++] = event_buf[event_buf_head];
        event_buf_head = (event_buf_head + 1) % MAX_EVENT_BUF;
        event_buf_count--;
    }
    return out;
}