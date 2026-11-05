#ifndef MV_TEXT_H
#define MV_TEXT_H
#include "graphics.h"

#define MV_GLYPH_W 8
#define MV_GLYPH_H 16

/* Draws ASCII text with the built-in 8x16 font, scaled by an integer factor. */
void ui_fill(Scene2D *s, int x, int y, int w, int h, Color c);
void ui_text(Scene2D *s, int x, int y, const char *str, int scale, Color c);
int  ui_text_width(const char *str, int scale);

#endif
