#include <body.h>
#include <math.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <util.h>

typedef int number;

static const unsigned int RANDOM_SEED = 3141592; // srand takes unsigned int

static const size_t NUM_STAR_POINTS_MIN = 2;  // inclusive
static const size_t NUM_STAR_POINTS_MAX = 10; // exclusive
static const double STAR_OUTER_RADIUS = 50;
static const double STAR_INNER_RADIUS = 20;

static const double STAR_MASS = 1.0;
static const vector_t STAR_INITIAL_VEL = {50.0, 0.0}; // pixels/s
static const double STAR_INITIAL_ANGULAR_VEL = -1.0;  // rad/s
static const double STAR_COEFFICIENT_RESTITUTION = 0.9;

static const vector_t GRAVITY = {0, -100}; // pixels/s^2

static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const size_t NUM_STARS = 200;

struct state {
  scene_t *scene;
};

static void new_star(state_t *state) {
  size_t num_points = rand() % (NUM_STAR_POINTS_MAX - NUM_STAR_POINTS_MIN) +
                      NUM_STAR_POINTS_MIN;
  list_t *star_shape =
      shape_star_create(num_points, STAR_OUTER_RADIUS, STAR_INNER_RADIUS);
  body_t *star = body_init(star_shape, STAR_MASS, color_random());

  vector_t start_pos = {STAR_OUTER_RADIUS, SCREEN_SIZE.y - STAR_OUTER_RADIUS};
  body_set_centroid(star, start_pos);

  body_set_velocity(star, STAR_INITIAL_VEL);
  body_set_angular_velocity(star, STAR_INITIAL_ANGULAR_VEL);

  scene_add_body(state->scene, star);
}

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);

  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();
  new_star(state);

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  vector_t gravity_delta = vec_multiply(dt, GRAVITY);
  vector_t bounds = {INFINITY, SCREEN_SIZE.y};

  size_t num_stars = scene_bodies(state->scene);
  for (size_t i = 0; i < num_stars; i++) {
    body_t *star = scene_get_body(state->scene, i);
    body_set_velocity(star, vec_add(body_get_velocity(star), gravity_delta));
    body_tick_with_bounds(star, dt, bounds, STAR_COEFFICIENT_RESTITUTION);
  }

  vector_t baby_star_center =
      body_get_centroid(scene_get_body(state->scene, num_stars - 1));
  if (baby_star_center.x > SCREEN_SIZE.x / NUM_STARS + STAR_OUTER_RADIUS) {
    new_star(state);
  }

  vector_t oldest_star_center =
      body_get_centroid(scene_get_body(state->scene, 0));
  if (oldest_star_center.x - STAR_OUTER_RADIUS > SCREEN_SIZE.x) {
    // star off screen
    scene_remove_body(state->scene, 0);
  }

  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}