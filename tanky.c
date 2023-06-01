#include <body.h>
#include <collision.h>
#include <color.h>
#include <forces.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>
#include <stdint.h>
#include <image.h>
#include <font.h>

static const unsigned int RANDOM_SEED = 12346; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const double EXTERIOR_WALL_THICKNESS = 100.0;

static const vector_t BULLET_SIZE = {20.0, 10.0};
static const double BULLET_MASS = 1.0;
static const vector_t BULLET_INITIAL_VEL = {250.0, -400.0};
static const rgb_color_t BULLET_COLOR = {1.0, 1.0, 0.0};
static const uint8_t BULLET_INFO =
    0; // address of BULLET_INFO specifies body is a bullet

static const vector_t TANK_SIZE = {100.0, 30.0};
static const double TANK_DRAG = 20.0;  // very high drag, so slows down almost instantly
static const double TANK_FORCE = 2000.0;
static const double TANK_ANGULAR_VEL = M_PI;
static const vector_t TANK_IMAGE_OFFSET = (vector_t) {0.0, 5.0};

static const vector_t TANK1_INITIAL_POSITION = {20.0, 450.0};
static const uint8_t TANK1_INFO =
    0; // address of TANK1_INFO specifies body is TANK1

static const vector_t TANK2_INITIAL_POSITION = {950.0, 20.0};
static const uint8_t TANK2_INFO =
    0; // address of TANK2_INFO specifies body is TANK2

static const double ELASTICITY = 1.0;

static const size_t POINTS_PER_BULLET = 1;

// old stuff for compiling
static const vector_t BRICK_SIZE = {89, 30};
static const vector_t BRICK_TOP_LEFT = {55, 475};
static const vector_t BRICK_SPACING = {99, 40};
static const size_t BRICK_COLS = 10;
static const size_t BRICK_ROWS = 3;
static const uint8_t BRICK_INFO =
    0; // address of BRICK_INFO specifies body is a brick

static const double BALL_RADIUS = 10;
static const double BALL_MASS = 10.0;
static const double BALL_Y = 67;
static const vector_t BALL_INITIAL_VEL = {250.0, -400.0};

static const vector_t PLAYER_SIZE = {100.0, 30.0};
static const double PLAYER_Y = 30.0;
static const double PLAYER_SPEED = 500.0; // pixels/s

static const double BULLET_Y = 80;
static const double BULLET_RADIUS = 20.0;
static const vector_t BULLET_VELOCITY = {0.0, 200.0};

typedef struct tank {
  body_t *body;
  list_t *health; // list of bodies that represent health bar
  list_t *powerups;
  bool just_shot;
  // image that correspondes to tank
  size_t points;
} tank_t;

typedef struct bullet {
  body_t *body;
  uint8_t target;
  uint8_t owner;
} bullet_t;

typedef struct map {
  list_t *walls;
  list_t *obstacles;
} map_t;

struct state {
  scene_t *scene;
  map_t map;
  tank_t tank_1;
  tank_t tank_2;
  list_t *bullets; // list of bullet_t bullets
};

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  image_init();
  font_init();
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();

  state->tank_1.body = body_init(shape_circle_create(20.0), 1.0, COLOR_WHITE);
  body_set_centroid(state->tank_1.body, vec_divide(2.0, SCREEN_SIZE));
  scene_add_body(state->scene, state->tank_1.body);
  body_set_image(state->tank_1.body, "tank_red", 1.0);
  body_set_image_rotation(state->tank_1.body, PI / 2);
  body_set_image_offset(state->tank_1.body, TANK_IMAGE_OFFSET);
  create_drag(state->scene, TANK_DRAG, state->tank_1.body);

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  if (sdl_get_key_pressed(UP_ARROW)) {
    vector_t force = vec_rotate((vector_t) {TANK_FORCE, 0.0}, body_get_angle(state->tank_1.body));
    body_add_force(state->tank_1.body, force);
  } else if (sdl_get_key_pressed(DOWN_ARROW)) {
    vector_t force = vec_rotate((vector_t) {-TANK_FORCE, 0.0}, body_get_angle(state->tank_1.body));
    body_add_force(state->tank_1.body, force);
  }

  if (sdl_get_key_pressed(RIGHT_ARROW)) {
    body_set_angular_velocity(state->tank_1.body, -TANK_ANGULAR_VEL);
  } else if (sdl_get_key_pressed(LEFT_ARROW)) {
    body_set_angular_velocity(state->tank_1.body, TANK_ANGULAR_VEL);
  } else {
    body_set_angular_velocity(state->tank_1.body, 0.0);
  }

  const size_t MAX_STR_SIZE = 256;
  char display_str[MAX_STR_SIZE];
  snprintf(display_str, MAX_STR_SIZE, "tank coords: %f,%f",
    body_get_centroid(state->tank_1.body).x,
    body_get_centroid(state->tank_1.body).y);
  vector_t text_top_left = {20, 480};
  scene_draw_text(state->scene, display_str, text_top_left, COLOR_WHITE);

  scene_tick(state->scene, dt);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
  image_deinit();
  font_deinit();
}