#ifndef WIDGET_WINDOW_H
#define WIDGET_WINDOW_H

#include "widget.h"

typedef struct
{
    int x, y, width, height;
    char *title;
    bool is_minimized;
    bool is_focussed;
    Widget_t *focused_widget;
    Widget_t *main_widget;
} Window_t;

#endif // WIDGET_WINDOW_H