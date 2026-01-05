#include "font.h"
#include <stddef.h>
#include <stdint.h>

extern uint8_t _binary_fonts_CascadiaCode_sfn_start[];
extern uint8_t _binary_fonts_CascadiaCode_sfn_end[];

static const void* g_font_data = NULL;

void font_init(void)
{
    if (_binary_fonts_CascadiaCode_sfn_start != _binary_fonts_CascadiaCode_sfn_end)
    {
        g_font_data = (const void*)_binary_fonts_CascadiaCode_sfn_start;
    }
    else
    {
        g_font_data = NULL;
    }
}

const void* font_get_sfn_data(void)
{
    return g_font_data;
}