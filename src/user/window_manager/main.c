#include "widgets/widget.h"
#include "widgets/window.h"

typedef struct WindowManager
{
    Window_t **windows;
    int num_windows;
    Widget_t *focussed_widget;
    Widget_t *focussed_window;
} WindowManager_t;

void _start(void)
{
    
}