#include <color.h>
#include <stdlib.h>
#include <util.h>

const rgb_color_t COLOR_WHITE = {1.0f, 1.0f, 1.0f};
static const double COLOR_VAL_MIN = 0.1;
static const double COLOR_VAL_MAX = 1.0;

rgb_color_t color_random() {
  rgb_color_t color = {rand_range(COLOR_VAL_MIN, COLOR_VAL_MAX),
                       rand_range(COLOR_VAL_MIN, COLOR_VAL_MAX),
                       rand_range(COLOR_VAL_MIN, COLOR_VAL_MAX)};
  return color;
}