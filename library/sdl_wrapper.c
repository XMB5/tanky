#include "sdl_wrapper.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <util.h>
#include <font.h>

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
long last_clock = 0;

static bool keys_pressed[CHAR_MAX + 1] = {false};

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  // this hint doesn't work in emscripten
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

bool sdl_is_done(void) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0') {
        // skip unknown key
        break;
      }
      keys_pressed[(size_t)key] = event->type == SDL_KEYDOWN;

      if (key_handler) {
        uint32_t timestamp = event->key.timestamp;
        if (!event->key.repeat) {
          key_start_timestamp = timestamp;
        }
        key_event_type_t type =
            event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
        double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
        key_handler(key, type, held_time);
      }
      break;
    }
    }
  }
  free(event);
  return false;
}

bool sdl_get_key_pressed(char key) { return keys_pressed[(size_t)key]; }

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, 255);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

void sdl_render_scene(scene_t *scene) {
  sdl_clear();
  vector_t window_center = get_window_center();

  // draw images
  list_t *images_to_draw = scene_get_images_to_draw(scene);
  size_t images_to_draw_len = list_size(images_to_draw);
  for (size_t i = 0; i < images_to_draw_len; i++) {
    image_to_draw_t *to_draw = list_remove(images_to_draw, 0);
    vector_t centroid_window = get_window_position(to_draw->pos, window_center);
    SDL_FRect dstrect;
    dstrect.h = to_draw->scale * to_draw->image->h;
    dstrect.w = to_draw->scale * to_draw->image->w;
    SDL_FPoint center;
    center.x = dstrect.w / 2.0;
    center.y = dstrect.h / 2.0;
    dstrect.x = centroid_window.x - center.x;
    dstrect.y = centroid_window.y - center.y;
    SDL_RenderCopyExF(renderer, to_draw->image->texture, NULL, &dstrect, -to_draw->rotation / PI * 180.0, &center, 0);
    free(to_draw);
  }

  // draw bodies
  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    image_t *image = body_get_image(body);

    if (image) {
      vector_t centroid_window = get_window_position(body_get_centroid(body), window_center);
      double scale = body_get_image_scale(body);
      vector_t offset = body_get_image_offset(body);
      SDL_FRect dstrect;
      dstrect.h = scale * image->h;
      dstrect.w = scale * image->w;
      SDL_FPoint center;
      center.x = dstrect.w / 2.0 + scale * offset.x;
      center.y = dstrect.h / 2.0 + scale * -offset.y;  // negative because screen space uses opposite convention
      dstrect.x = centroid_window.x - center.x;
      dstrect.y = centroid_window.y - center.y;
      double rot = body_get_angle(body) + body_get_image_rotation(body);
      SDL_RenderCopyExF(renderer, image->texture, NULL, &dstrect, -rot / PI * 180.0, &center, 0);
    } else {
      list_t *shape = body_get_shape_unsafe(body);
      sdl_draw_polygon(shape, body_get_color(body));
    }
  }

  // draw text
  list_t *texts_to_draw = scene_get_texts_to_draw(scene);
  size_t texts_to_draw_len = list_size(texts_to_draw);
  for (size_t i = 0; i < texts_to_draw_len; i++) {
    text_to_draw_t *to_draw = list_remove(texts_to_draw, 0);;
    font_render(to_draw->text, get_window_position(to_draw->top_left, window_center), to_draw->color);
    scene_text_to_draw_free(to_draw);
  }
  
  sdl_show();
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  long now = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}

SDL_Renderer *sdl_renderer() {
  return renderer;
}