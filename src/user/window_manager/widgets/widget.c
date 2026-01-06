#include "widget.h"

Widget_t *widget_init(int x, int y, int width, int height, Widget_t *parent)
{
    
}

void widget_draw(Widget_t *self)
{
    draw_children(self);
}