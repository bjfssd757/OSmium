#ifndef WIDGET_TEXTBLOCK_H
#define WIDGET_TEXTBLOCK_H

#include "../../theme.h"
#include "../widget.h"

typedef struct TextBlock
{
    Widget_t base;
    char *current_text;
} TextBlock_t;

void textblock_init(TextBlock_t *textblock);
void textblock_draw(Widget_t *self);

#endif // WIDGET_TEXTBLOCK_H