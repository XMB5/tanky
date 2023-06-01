#ifndef __FONT_H__
#define __FONT_H__

#include <color.h>
#include <vector.h>

void font_init();
void font_render(const char *text, vector_t top_left, rgb_color_t color);
void font_deinit();

#endif