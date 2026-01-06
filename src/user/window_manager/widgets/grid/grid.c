#include "grid.h"

void grid_init(Grid_t *grid)
{
    grid->base.draw = grid_draw;
}

void grid_draw(Widget_t *self)
{
    Grid_t *g = (Grid_t*)self;
    // ...
    draw_children(self);
}