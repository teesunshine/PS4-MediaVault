#include "text.h"
#include "font8x16.h"
#include <string.h>

#define SCREEN_W 1920
#define SCREEN_H 1080

void ui_fill(Scene2D *s, int x, int y, int w, int h, Color c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;
    s->DrawRectangle(x, y, w, h, c);
}

int ui_text_width(const char *str, int scale) {
    return (int)strlen(str) * MV_GLYPH_W * scale;
}

void ui_text(Scene2D *s, int x, int y, const char *str, int scale, Color c) {
    for (; *str; str++, x += MV_GLYPH_W * scale) {
        unsigned char ch = (unsigned char)*str;
        if (ch < 32 || ch > 126) ch = '?';
        const unsigned char *g = MV_FONT8X16[ch - 32];
        for (int row = 0; row < MV_GLYPH_H; row++) {
            unsigned char bits = g[row];
            int col = 0;
            while (col < MV_GLYPH_W) {
                if (!(bits & (0x80 >> col))) { col++; continue; }
                int run = 1;
                while (col + run < MV_GLYPH_W && (bits & (0x80 >> (col + run)))) run++;
                ui_fill(s, x + col * scale, y + row * scale, run * scale, scale, c);
                col += run;
            }
        }
    }
}
