#ifndef __COLOR_H__
#define __COLOR_H__

#include <SDL2/SDL.h>

/**
 * A color to display on the screen.
 * The color is represented by its red, green, and blue components.
 * Each component must be between 0 (black) and 1 (white).
 */
typedef struct {
  float r;
  float g;
  float b;
} rgb_color_t;

rgb_color_t color_random();
SDL_Color color_to_sdl(rgb_color_t color);

extern const rgb_color_t COLOR_WHITE;

#endif