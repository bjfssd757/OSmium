#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>
#include "../limine.h"

/**
 * @brief Initializes the graphics subsystem.
 * @param fb Pointer to a framebuffer_info_t structure from the loader.
 */
void gfx_init(struct limine_framebuffer *fb);

/**
 * @brief Copies backbuffer requests to the visible screen.
 */
void gfx_update_screen(void);

/**
 * @brief Clears the entire backbuffer (fills with one color).
 * @param color Color in ARGB format (e.g. 0x00FF0000 for red).
 */
void gfx_clear(uint32_t color);

/**
 * @brief Draws a line of text at the specified coordinates.
 * 
 * @param x Initial coordinate X.
 * @param y The initial Y coordinate (for the font baseline).
 * @param size Font size in pixels.
 * @param color Text color in ARGB format.
 * @param s String to render (UTF-8).
 * @return The final X coordinate after drawing the line.
 */
int gfx_draw_string(int x, int y, int size, uint32_t color, const char *s);

/**
 * @brief Calculates the bounding rectangle for a string without drawing it.
 * 
 * @param size The font size to be used.
 * @param s Line to measure.
 * @param w Pointer to return width.
 * @param h Pointer to return height.
 */
void gfx_get_text_bounds(int size, const char *s, int *w, int *h);

/**
 * @brief Draws a point in the backbuffer.
 */
void gfx_draw_point(uint32_t x, uint32_t y, uint32_t color);

/**
 * @brief Draws a line using Bresenham's algorithm.
 */
void gfx_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

/**
 * @brief Draws the outline of a circle.
 */
void gfx_draw_circle(int32_t xc, int32_t yc, int32_t radius, uint32_t color);

/**
 * @brief Draws a filled circle.
 */
void gfx_fill_circle(int32_t xc, int32_t yc, int32_t radius, uint32_t color);

/**
 * @brief Draws the outline of a rectangle.
 */
void gfx_draw_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

/**
 * @brief Draws a filled rectangle.
 */
void gfx_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

/**
* @brief Draw a window
*/
void gfx_draw_window(int x, int y, int w, int h, const char *title);

#endif /* GRAPHICS_H */