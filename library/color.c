#include <color.h>
#include <stdlib.h>
#include <util.h>

const rgb_color_t COLOR_WHITE = {1.0f, 1.0f, 1.0f};
static const double COLOR_VAL_MIN = 0.1;
static const double COLOR_VAL_MAX = 1.0;
static const int SDL_COLOR_MAX = 255;

rgb_color_t color_random() {
  rgb_color_t color = {rand_range(COLOR_VAL_MIN, COLOR_VAL_MAX),
                       rand_range(COLOR_VAL_MIN, COLOR_VAL_MAX),
                       rand_range(COLOR_VAL_MIN, COLOR_VAL_MAX)};
  return color;
}

SDL_Color color_to_sdl(rgb_color_t color) {
  SDL_Color sdl_color;
  sdl_color.r = (int) (color.r * SDL_COLOR_MAX);
  sdl_color.g = (int) (color.g * SDL_COLOR_MAX);
  sdl_color.b = (int) (color.b * SDL_COLOR_MAX);
  sdl_color.a = SDL_COLOR_MAX;
  return sdl_color;
}