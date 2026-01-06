#ifndef WIDGET_GRID_H
#define WIDGET_GRID_H

#include "../../theme.h"
#include "../widget.h"

typedef struct
{
    Widget_t base;
    Widget_t **children;
    int rows, cols;
} Grid_t;

void grid_init(Grid_t *grid);
void grid_draw(Widget_t *self);

#endif // WIDGET_GRID_H