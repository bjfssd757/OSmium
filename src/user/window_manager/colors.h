#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

#define COLOR_RGBA(r,g,b,a) \
    ((uint32_t)((((uint32_t)(a) & 0xFFu) << 24) | (((uint32_t)(r) & 0xFFu) << 16) | (((uint32_t)(g) & 0xFFu) << 8) | ((uint32_t)(b) & 0xFFu)))

#define COLOR_RGB_HEX(rgb) ((uint32_t)((((uint32_t)0xFFu) << 24) | (((uint32_t)(rgb) & 0xFF0000u)) | (((uint32_t)(rgb) & 0x00FF00u)) | (((uint32_t)(rgb) & 0x0000FFu))))

#define COLOR_WITH_RESERVED(col, a) \
    ((uint32_t)((((uint32_t)(a) & 0xFFu) << 24) | ((uint32_t)(col) & 0x00FFFFFFu)))

#define COLOR_GET_BLUE(col)  ((uint8_t)((col) & 0xFFu))
#define COLOR_GET_GREEN(col) ((uint8_t)(((col) >> 8) & 0xFFu))
#define COLOR_GET_RED(col)   ((uint8_t)(((col) >> 16) & 0xFFu))
#define COLOR_GET_RESERVED(col) ((uint8_t)(((col) >> 24) & 0xFFu))

#define COLOR_WITH_ALPHA(col, a)        ( \
    ((col) & 0x00FFFFFFu) | (((uint32_t)(a) & 0xFFu) << 24) \
)
#define GLOW(col)       COLOR_WITH_ALPHA(col, 153)
#define SHADOW(col)     COLOR_WITH_ALPHA(col, 38)
#define SHADOW2(col)    COLOR_WITH_ALPHA(col, 80)
#define SHADOW3(col)    COLOR_WITH_ALPHA(col, 120)

#define COLOR_TRANSPARENT  ((uint32_t)0x00000000u)
#define COLOR_BLACK        ((uint32_t)0xFF000000u) /* #000000 */
#define COLOR_WHITE        ((uint32_t)0xFFFFFFFFu) /* #FFFFFF */
#define COLOR_RED          ((uint32_t)0xFFFF0000u) /* #FF0000 */
#define COLOR_GREEN        ((uint32_t)0xFF00FF00u) /* #00FF00 */
#define COLOR_BLUE         ((uint32_t)0xFF0000FFu) /* #0000FF */
#define COLOR_CYAN         ((uint32_t)0xFF00FFFFu) /* #00FFFF */
#define COLOR_MAGENTA      ((uint32_t)0xFFFF00FFu) /* #FF00FF */
#define COLOR_YELLOW       ((uint32_t)0xFFFFFF00u) /* #FFFF00 */
#define COLOR_ORANGE       ((uint32_t)0xFFFFA500u) /* #FFA500 */
#define COLOR_PURPLE       ((uint32_t)0xFF800080u) /* #800080 */
#define COLOR_PINK         ((uint32_t)0xFFFFC0CBu) /* #FFC0CB */
#define COLOR_BROWN        ((uint32_t)0xFFA52A2Au) /* #A52A2A */
#define COLOR_GRAY         ((uint32_t)0xFF808080u) /* #808080 */
#define COLOR_LIGHT_GRAY   ((uint32_t)0xFFD3D3D3u) /* #D3D3D3 */
#define COLOR_DARK_GRAY    ((uint32_t)0xFFA9A9A9u) /* #A9A9A9 */
#define COLOR_NAVY         ((uint32_t)0xFF000080u) /* #000080 */
#define COLOR_TEAL         ((uint32_t)0xFF008888u) /* approximated #008080 */
#define COLOR_OLIVE        ((uint32_t)0xFF808000u) /* #808000 */
#define COLOR_MAROON       ((uint32_t)0xFF800000u) /* #800000 */
#define COLOR_LIME         ((uint32_t)0xFF00FF00u) /* #00FF00 */
#define COLOR_SILVER       ((uint32_t)0xFFC0C0C0u) /* #C0C0C0 */
#define COLOR_GOLD         ((uint32_t)0xFFFFD700u) /* #FFD700 */
#define COLOR_SKY_BLUE     ((uint32_t)0xFF87CEEBu) /* #87CEEB */
#define COLOR_CORAL        ((uint32_t)0xFFFF7F50u) /* #FF7F50 */
#define COLOR_SALMON       ((uint32_t)0xFFFA8072u) /* #FA8072 */
#define COLOR_KHAKI        ((uint32_t)0xFFF0E68Cu) /* #F0E68C */
#define COLOR_BEIGE        ((uint32_t)0xFFF5F5DCu) /* #F5F5DC */
#define COLOR_DODGER_BLUE  ((uint32_t)0xFF1E90FFu) /* #1E90FF */
#define COLOR_INDIGO       ((uint32_t)0xFF4B0082u) /* #4B0082 */
#define COLOR_TOMATO       ((uint32_t)0xFFFF6347u) /* #FF6347 */
#define COLOR_CHARTREUSE   ((uint32_t)0xFF7FFF00u) /* #7FFF00 */
#define COLOR_TURQUOISE    ((uint32_t)0xFF40E0D0u) /* #40E0D0 */

#endif /* COLORS_H */