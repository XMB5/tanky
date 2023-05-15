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

static const unsigned int RANDOM_SEED = 12346; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const double WALL_THICKNESS = 100.0;

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
static const double BULLET_MASS = 5.0;
static const vector_t BULLET_VELOCITY = {0.0, 200.0};
static const rgb_color_t BULLET_COLOR = {1.0, 1.0, 0.0};
static const uint8_t BULLET_INFO =
    0; // address of BULLET_INFO specifies body is a bullet

static const double ELASTICITY = 1.0;

static const size_t POINTS_PER_BULLET = 3;

struct state {
  scene_t *scene;
  body_t *player;
  body_t *ball;
  bool just_shot;
  size_t bricks_killed;
  size_t points; // carry over between rounds
};

static void brick_collision_handler(body_t *brick, body_t *ball, vector_t axis,
                                    state_t *state) {
  physics_collision_handler(brick, ball, axis, &ELASTICITY);
  body_remove(brick);
  state->bricks_killed++;
  state->points++;
}

static void reset(state_t *state) {
  body_set_centroid(state->player, (vector_t){SCREEN_SIZE.x / 2.0, PLAYER_Y});

  body_set_centroid(state->ball, (vector_t){SCREEN_SIZE.x / 2.0, BALL_Y});
  body_set_velocity(state->ball, BALL_INITIAL_VEL);

  // remove old bricks and bullets
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    void *info = body_get_info(body);
    if (info == &BRICK_INFO || info == &BULLET_INFO) {
      body_remove(body);
    }
  }

  // add new bricks
  for (size_t x = 0; x < BRICK_COLS; x++) {
    for (size_t y = 0; y < BRICK_ROWS; y++) {
      body_t *brick =
          body_init_with_info(shape_rectangle(BRICK_SIZE), INFINITY,
                              color_random(), (void *)&BRICK_INFO, NULL);
      vector_t offset = {BRICK_SPACING.x * x, -BRICK_SPACING.y * y};
      body_set_centroid(brick, vec_add(BRICK_TOP_LEFT, offset));
      scene_add_body(state->scene, brick);
      create_collision(state->scene, brick, state->ball,
                       (collision_handler_t)brick_collision_handler, state,
                       NULL);
    }
  }
  state->bricks_killed = 0;
}

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  srand(RANDOM_SEED);

  printf("every 3 bricks killed, you get one pacman! press space to shoot\n");

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();
  state->player = body_init_with_info(shape_rectangle(PLAYER_SIZE), INFINITY,
                                      color_random(), NULL, NULL);
  scene_add_body(state->scene, state->player);
  state->just_shot = false;

  state->ball = body_init_with_info(shape_circle_create(BALL_RADIUS), BALL_MASS,
                                    color_random(), NULL, NULL);
  scene_add_body(state->scene, state->ball);

  // ceiling
  body_t *walls[] = {
      body_init_with_info(
          shape_rectangle((vector_t){SCREEN_SIZE.x, WALL_THICKNESS}), INFINITY,
          COLOR_WHITE, NULL, NULL),
      body_init_with_info(
          shape_rectangle((vector_t){WALL_THICKNESS, SCREEN_SIZE.y}), INFINITY,
          COLOR_WHITE, NULL, NULL),
      body_init_with_info(
          shape_rectangle((vector_t){WALL_THICKNESS, SCREEN_SIZE.y}), INFINITY,
          COLOR_WHITE, NULL, NULL)};
  body_set_centroid(walls[0], (vector_t){SCREEN_SIZE.x / 2.0,
                                         SCREEN_SIZE.y + WALL_THICKNESS / 2.0});
  body_set_centroid(walls[1],
                    (vector_t){-WALL_THICKNESS / 2.0, SCREEN_SIZE.y / 2.0});
  body_set_centroid(walls[2], (vector_t){SCREEN_SIZE.x + WALL_THICKNESS / 2.0,
                                         SCREEN_SIZE.y / 2.0});

  const size_t num_walls = 3;
  for (size_t i = 0; i < num_walls; i++) {
    scene_add_body(state->scene, walls[i]);
    create_physics_collision(state->scene, ELASTICITY, state->ball, walls[i]);
  }

  create_physics_collision(state->scene, ELASTICITY, state->ball,
                           state->player);

  reset(state);

  return state;
}

static void shoot_bullet(state_t *state) {
  body_t *bullet =
      body_init_with_info(shape_pacman_create(BULLET_RADIUS), BULLET_MASS,
                          BULLET_COLOR, (void *)&BULLET_INFO, NULL);
  body_set_rotation(bullet, PI / 2); // face up
  body_set_centroid(bullet,
                    (vector_t){body_get_centroid(state->player).x, BULLET_Y});
  body_set_velocity(bullet, BULLET_VELOCITY);

  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (body_get_info(body) == &BRICK_INFO) {
      create_collision(state->scene, body, bullet,
                       (collision_handler_t)brick_collision_handler, state,
                       NULL);
    }
  }
  create_physics_collision(state->scene, ELASTICITY, state->ball, bullet);
  create_physics_collision(state->scene, ELASTICITY, state->player, bullet);
  scene_add_body(state->scene, bullet);
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  if (sdl_get_key_pressed(RIGHT_ARROW)) {
    body_set_velocity(state->player, (vector_t){PLAYER_SPEED, 0.0});
  } else if (sdl_get_key_pressed(LEFT_ARROW)) {
    body_set_velocity(state->player, (vector_t){-PLAYER_SPEED, 0.0});
  } else {
    body_set_velocity(state->player, VEC_ZERO);
  }

  if (sdl_get_key_pressed(' ')) {
    if (!state->just_shot) {
      state->just_shot = true;
      if (state->points >= POINTS_PER_BULLET) {
        state->points -= POINTS_PER_BULLET;
        shoot_bullet(state);
      }
    }
  } else {
    state->just_shot = false;
  }

  if (body_get_centroid(state->ball).y < 0) {
    reset(state);
  }

  if (state->bricks_killed == BRICK_COLS * BRICK_ROWS) {
    reset(state);
  }

  scene_tick(state->scene, dt);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}