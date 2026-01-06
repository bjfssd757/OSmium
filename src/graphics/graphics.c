#include "vga.h"
#define SSFN_IMPLEMENTATION
#include <stdint.h>
#include "sfn.h"
#include "graphics.h"
#include "font.h"
#include <stddef.h>
#include "../malloc/malloc.h"
#include "../libc/string.h"
#include "formatting.h"
#include "colors.h"
#include "../time/timer.h"

static struct limine_framebuffer *g_fb = NULL;
static uint8_t *g_backbuffer = NULL;
static ssfn_t g_ssfn_ctx;

extern struct limine_hhdm_response *hhdm_res;

static inline bool in_bounds(int32_t x, int32_t y)
{
    if (!g_fb) return false;
    if (x < 0 || y < 0) return false;
    if ((uint32_t)x >= g_fb->width || (uint32_t)y >= g_fb->height) return false;
    return true;
}

static void gfx_put_pixel_backbuffer(uint32_t x, uint32_t y, uint32_t color)
{
    if (!in_bounds((int32_t)x, (int32_t)y)) return;

    uint8_t *ptr = g_backbuffer + y * g_fb->pitch + x * (g_fb->bpp / 8);

    if (g_fb->bpp == 32)
    {
        *(uint32_t *)ptr = color;
    }
    else if (g_fb->bpp == 24)
    {
        ptr[0] = (uint8_t)(color & 0xFF);        // Blue
        ptr[1] = (uint8_t)((color >> 8) & 0xFF); // Green
        ptr[2] = (uint8_t)((color >> 16) & 0xFF);// Red
    }
}


void gfx_init(struct limine_framebuffer *fb)
{
    g_fb = fb;
    if (!g_fb) return;

    size_t backbuffer_size = g_fb->pitch * g_fb->height;
    g_backbuffer = (uint8_t *)malloc(backbuffer_size);
    if (!g_backbuffer)
    {
        return;
    }
    memset(g_backbuffer, 0, backbuffer_size);

    font_init();
    const void *font_data = font_get_sfn_data();
    if (!font_data) {
        return;
    }

    memset(&g_ssfn_ctx, 0, sizeof(ssfn_t));
    int res = ssfn_load(&g_ssfn_ctx, font_data);
    if (res != SSFN_OK)
    {
        gfx_draw_rect(30, 30, 60, 60, COLOR_BLACK);
        return;
    }
}

void gfx_update_screen(void)
{
    if (!g_fb || !g_backbuffer) return;

    uint8_t *fb_ptr = (uint8_t *)((uint64_t)g_fb->address);
    memcpy(fb_ptr, g_backbuffer, g_fb->pitch * g_fb->height);
}

int gfx_draw_string(int x, int y, int size, uint32_t color, const char *s)
{
    if (!g_fb || !s) return x;

    int ret = ssfn_select(&g_ssfn_ctx, SSFN_FAMILY_ANY, NULL, SSFN_STYLE_REGULAR, size);
    if (ret < 0)
        return ret;

    ssfn_buf_t dest_buf = {
        .ptr = g_backbuffer,
        .w = g_fb->width,
        .h = g_fb->height,
        .p = g_fb->pitch,
        .x = x,
        .y = y,
        .fg = color,
        .bg = 0
    };

    while (*s)
    {
        ret = ssfn_render(&g_ssfn_ctx, &dest_buf, s);
        if (ret < 0)
            return ret;

        s += (ret > 0) ? ret : 1;
    }

    return dest_buf.x;
}

void gfx_get_text_bounds(int size, const char *s, int *w, int *h)
{
    if (!s || !w || !h) return;

    ssfn_select(&g_ssfn_ctx, SSFN_FAMILY_ANY, NULL, SSFN_STYLE_REGULAR, size);
    ssfn_bbox(&g_ssfn_ctx, s, w, h, NULL, NULL);
}

void gfx_clear(uint32_t color)
{
    if (!g_fb || !g_backbuffer) return;
    gfx_fill_rect(0, 0, g_fb->width - 1, g_fb->height - 1, color);
}

static void draw_hline_clipped(int32_t x0, int32_t x1, int32_t y, uint32_t color)
{
    if (!g_fb || !g_backbuffer) return;
    if (y < 0 || (uint32_t)y >= g_fb->height) return;

    if (x0 > x1) { int32_t tmp = x0; x0 = x1; x1 = tmp; }

    if (x1 < 0 || (uint32_t)x0 >= g_fb->width) return;
    if (x0 < 0) x0 = 0;
    if ((uint32_t)x1 >= g_fb->width) x1 = g_fb->width - 1;

    uint8_t* row_start = g_backbuffer + (uint64_t)y * g_fb->pitch;

    if (g_fb->bpp == 32)
    {
        uint32_t *dst = (uint32_t *)(row_start + (uint64_t)x0 * 4);
        for (int32_t x = x0; x <= x1; ++x)
        {
            *dst++ = color;
        }
    }
    else if (g_fb->bpp == 24)
    {
        uint8_t* dst = row_start + (uint64_t)x0 * 3;
        uint8_t b = color & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t r = (color >> 16) & 0xFF;
        for (int32_t x = x0; x <= x1; ++x)
        {
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
        }
    }
}

void gfx_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    if (!g_fb) return;

    if (x0 > x1) { int32_t t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int32_t t = y0; y0 = y1; y1 = t; }

    for (int32_t y = y0; y <= y1; ++y)
    {
        draw_hline_clipped(x0, x1, y, color);
    }
}

void gfx_draw_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    if (!g_fb) return;

    if (x0 > x1) { int32_t t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int32_t t = y0; y0 = y1; y1 = t; }

    draw_hline_clipped(x0, x1, y0, color);
    draw_hline_clipped(x0, x1, y1, color);
    gfx_draw_line(x0, y0, x0, y1, color);
    gfx_draw_line(x1, y0, x1, y1, color);
}

void gfx_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int32_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int32_t sx = (x1 > x0) ? 1 : ((x1 < x0) ? -1 : 0);
    int32_t sy = (y1 > y0) ? 1 : ((y1 < y0) ? -1 : 0);

    if (dx == 0) {
        if (y0 > y1) { int32_t t = y0; y0 = y1; y1 = t; }
        for (int32_t y = y0; y <= y1; y++) {
            gfx_put_pixel_backbuffer(x0, y, color);
        }
        return;
    }
    
    if (dy == 0) {
        draw_hline_clipped(x0, x1, y0, color);
        return;
    }

    int32_t err = dx - dy;

    while(true)
    {
        gfx_put_pixel_backbuffer(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void gfx_draw_circle(int32_t xc, int32_t yc, int32_t radius, uint32_t color)
{
    if (radius < 0) return;
    int32_t x = 0;
    int32_t y = radius;
    int32_t d = 3 - 2 * radius;

    while (y >= x)
    {
        gfx_put_pixel_backbuffer(xc + x, yc + y, color);
        gfx_put_pixel_backbuffer(xc - x, yc + y, color);
        gfx_put_pixel_backbuffer(xc + x, yc - y, color);
        gfx_put_pixel_backbuffer(xc - x, yc - y, color);
        gfx_put_pixel_backbuffer(xc + y, yc + x, color);
        gfx_put_pixel_backbuffer(xc - y, yc + x, color);
        gfx_put_pixel_backbuffer(xc + y, yc - x, color);
        gfx_put_pixel_backbuffer(xc - y, yc - x, color);
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
}

void gfx_fill_circle(int32_t xc, int32_t yc, int32_t radius, uint32_t color)
{
    if (radius < 0) return;
    int32_t x = 0;
    int32_t y = radius;
    int32_t d = 3 - 2 * radius;
    
    while (y >= x)
    {
        draw_hline_clipped(xc - x, xc + x, yc + y, color);
        draw_hline_clipped(xc - x, xc + x, yc - y, color);
        draw_hline_clipped(xc - y, xc + y, yc + x, color);
        draw_hline_clipped(xc - y, xc + y, yc - x, color);
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
}

void gfx_draw_window(int x, int y, int w, int h, const char *title)
{
    gfx_fill_rect(x, y, x + w, y + h, 0xAA333333);
    gfx_draw_rect(x, y, x + w, y + h, 0x00888888);
    gfx_fill_rect(x + 1, y + 1, x + w - 1, y + 20, 0x00555577);
    gfx_draw_string(x + 5, y + 16, 16, 0x00FFFFFF, title);
}

void gfx_draw_point(uint32_t x, uint32_t y, uint32_t color)
{
    gfx_put_pixel_backbuffer(x, y, color);
}