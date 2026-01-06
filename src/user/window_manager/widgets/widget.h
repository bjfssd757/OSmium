#ifndef WIDGET_H
#define WIDGET_H

#include "stdbool.h"

typedef struct Widget
{
    int x, y, width, height;
    struct Widget *parent;
    struct Widget **children;
    int num_children;

    void (*draw)(struct Widget *self);
    
    void (*OnClick)(struct Widget *self);
    void (*OnFocus)(struct Widget *self);
    void (*OnFocusEnd)(struct Widget *self);
    void (*OnKeyDown)(struct Widget *self, int key);
} Widget_t;

typedef enum
{
    WIDGET_EVENT_CLICK,
    WIDGET_EVENT_FOCUS,
    WIDGET_EVENT_FOCUS_END,
    WIDGET_EVENT_KEYDOWN
} WidgetEvent_t;

Widget_t *widget_init(int x, int y, int width, int height, Widget_t *parent);
void widget_draw(Widget_t *self);

static inline void draw_children(Widget_t *self)
{
    for (int i = 0; i < self->num_children; i++)
    {
        Widget_t *child = self->children[i];
        if (child && child->draw)
        {
            child->draw(child);
        }
    }
}

static inline bool call_handler(Widget_t *widget, WidgetEvent_t event, int key)
{
    while (widget)
    {
        switch (event)
        {
            case WIDGET_EVENT_CLICK:
                if (widget->OnClick) { widget->OnClick(widget); return true; }
                break;
            case WIDGET_EVENT_FOCUS_END:
                if (widget->OnFocusEnd) { widget->OnFocusEnd(widget); return true; }
                break;
            case WIDGET_EVENT_FOCUS:
                if (widget->OnFocus) { widget->OnFocus(widget); return true; }
                break;
            case WIDGET_EVENT_KEYDOWN:
                if (widget->OnKeyDown) { widget->OnKeyDown(widget, key); return true; }
                break;
        }
        widget = widget->parent;
    }
    return false;
}

#endif // WIDGET_H