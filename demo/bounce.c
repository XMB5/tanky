#include <body.h>
#include <color.h>
#include <math.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <stdbool.h>
#include <util.h>

static const unsigned int RANDOM_SEED = 42069; // srand() takes unsigned int

static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const size_t STAR_INNER_RADIUS = 20;
static const size_t STAR_POINTS = 5;
static const size_t STAR_OUTER_RADIUS = 50;

static const double STAR_MASS = 1.0;
static const vector_t STAR_INITIAL_VELOCITY = {200.0, 150.0}; // pixel/s
static const double STAR_INITIAL_ANGULAR_VELOCITY = 1.5;      // rad/s
static const double STAR_BOUNCINESS = 1.0;

struct state {
  scene_t *scene;
  body_t *star;
};

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();
  list_t *star_shape =
      shape_star_create(STAR_POINTS, STAR_OUTER_RADIUS, STAR_INNER_RADIUS);
  state->star = body_init(star_shape, STAR_MASS, color_random());
  body_set_velocity(state->star, STAR_INITIAL_VELOCITY);
  body_set_angular_velocity(state->star, STAR_INITIAL_ANGULAR_VELOCITY);
  scene_add_body(state->scene, state->star);

  vector_t screen_center = vec_multiply(0.5, SCREEN_SIZE);
  body_set_centroid(state->star, screen_center);

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  body_tick_with_bounds(state->star, dt, SCREEN_SIZE, STAR_BOUNCINESS);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}