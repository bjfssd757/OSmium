#ifndef EVENTBUF_H
#define EVENTBUF_H

#define MAX_EVENT_BUF 1024

typedef enum
{
    EVENT_MOUSE,
    EVENT_KEY,
    EVENT_WINDOW
} event_type_t;

typedef struct
{
    event_type_t type;
    union
    {
        struct
        {
            int x, y, button, state;
        } mouse;
        struct
        {
            int key, pressed;
        } key;
    } data;
    int target_wid;
    int pid;
} event_t;

extern event_t event_buf[MAX_EVENT_BUF];
extern int event_buf_head, event_buf_tail, event_buf_count;

void k_put_event(const event_t *e);
int sys_get_events(event_t *dst, size_t max_count);

#endif // EVENTBUF_H