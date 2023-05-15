#include <body.h>
#include <collision.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <util.h>
#include <vector.h>

static const unsigned int RANDOM_SEED = 1980; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const double PACMAN_RADIUS = 60.0;
static const double PACMAN_MASS = 1.0;
static const rgb_color_t PACMAN_COLOR = {1.0f, 1.0f, 0.0f};
static const double PACMAN_ACCEL = 100.0;

static const double PELLET_RADIUS = 10.0;
static const double PELLET_MASS = 69.0;
static const size_t NUM_INITIAL_PELLETS = 69;
static const double PELLETS_PER_SECOND = 2.0;

struct state {
  scene_t *scene;
  body_t *pacman;
};

static void spawn_pellet(state_t *state) {
  list_t *shape;
  if (rand() % 2) {
    shape = shape_pacman_create(PELLET_RADIUS);
  } else {
    shape = shape_circle_create(PELLET_RADIUS);
  }
  body_t *pellet = body_init(shape, PELLET_MASS, color_random());
  vector_t pos = {fmod(rand(), SCREEN_SIZE.x), fmod(rand(), SCREEN_SIZE.y)};
  body_set_centroid(pellet, pos);
  scene_add_body(state->scene, pellet);
}

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();
  state->pacman =
      body_init(shape_pacman_create(PACMAN_RADIUS), PACMAN_MASS, PACMAN_COLOR);
  body_set_centroid(state->pacman, vec_multiply(0.5, SCREEN_SIZE));
  scene_add_body(state->scene, state->pacman);

  for (size_t i = 0; i < NUM_INITIAL_PELLETS; i++) {
    spawn_pellet(state);
  }

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  double new_angle = NAN;
  vector_t new_vel = VEC_ZERO;
  if (sdl_get_key_pressed(UP_ARROW)) {
    new_angle = PI / 2;
    new_vel.y = body_get_velocity(state->pacman).y + PACMAN_ACCEL * dt;
  } else if (sdl_get_key_pressed(DOWN_ARROW)) {
    new_angle = -PI / 2;
    new_vel.y = body_get_velocity(state->pacman).y - PACMAN_ACCEL * dt;
  } else if (sdl_get_key_pressed(LEFT_ARROW)) {
    new_angle = PI;
    new_vel.x = body_get_velocity(state->pacman).x - PACMAN_ACCEL * dt;
  } else if (sdl_get_key_pressed(RIGHT_ARROW)) {
    new_angle = 0;
    new_vel.x = body_get_velocity(state->pacman).x + PACMAN_ACCEL * dt;
  }

  if (new_angle == new_angle) {
    // dont change angle for NAN
    body_set_rotation(state->pacman, new_angle);
  }
  body_set_velocity(state->pacman, new_vel);

  scene_tick(state->scene, dt);

  // wrap around
  vector_t pacman_pos = body_get_centroid(state->pacman);
  pacman_pos.x = fmod((pacman_pos.x + SCREEN_SIZE.x), SCREEN_SIZE.x);
  pacman_pos.y = fmod((pacman_pos.y + SCREEN_SIZE.y), SCREEN_SIZE.y);
  body_set_centroid(state->pacman, pacman_pos);

  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (body != state->pacman) {
      collision_info_t collision = body_collide(state->pacman, body);
      if (collision.collided) {
        scene_remove_body(state->scene, i);
        i--;
        num_bodies--;
      }
    }
  }

  if (((double)rand()) / RAND_MAX < PELLETS_PER_SECOND * dt) {
    spawn_pellet(state);
  }

  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}