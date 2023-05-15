#include <body.h>
#include <collision.h>
#include <color.h>
#include <emscripten.h>
#include <forces.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>

static const unsigned int RANDOM_SEED = 1978; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const double ENEMY_RADIUS = 40.0;
static const double ENEMY_EMPTY_ANGLE = 4.0;
static const double ENEMY_MASS = 1.0;
static const size_t ENEMY_ROWS = 3;
static const size_t ENEMY_COLS = 7;
static const vector_t ENEMY_SPAWN_TOP_LEFT = {100.0, 450.0};
static const vector_t ENEMY_SPACING = {100.0, 50.0};
static const vector_t ENEMY_VELOCITY = {100.0, 0.0};
static const uint8_t ENEMY_INFO =
    0; // address of this constant used for ENEMY_INFO
static const double ENEMY_WALL_DISPLACEMENT = 5.0;
static const double ENEMY_BULLETS_PER_SECOND = 0.01;

static const vector_t BULLET_SIZE = {6.0, 30.0};
static const double BULLET_MASS = 1.0;
static const double BULLET_SPEED = 300.0; // pixels/s

static const vector_t PLAYER_SIZE = {50.0, 10.0};
static const double PLAYER_MASS = 69.0;
static const double PLAYER_Y = 30.0;
static const double PLAYER_SPEED = 300.0; // pixels/s

static const double SHOOT_INTERVAL = 0.15; // sec

struct state {
  scene_t *scene;
  body_t *player;
  double shoot_cooldown;
};

static void gameover() {
  // exit(0) does not end emscripten
  emscripten_force_exit(0);
}

static void player_free(void *info) { gameover(); }

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();
  state->player =
      body_init_with_info(shape_ellipse(PLAYER_SIZE), PLAYER_MASS,
                          color_random(), &player_free, player_free);
  state->shoot_cooldown = 0.0;
  body_set_centroid(state->player, (vector_t){SCREEN_SIZE.x / 2.0, PLAYER_Y});
  scene_add_body(state->scene, state->player);

  for (size_t x = 0; x < ENEMY_COLS; x++) {
    for (size_t y = 0; y < ENEMY_ROWS; y++) {
      body_t *enemy = body_init_with_info(
          shape_arc_sweep(ENEMY_RADIUS, ENEMY_EMPTY_ANGLE), ENEMY_MASS,
          color_random(), (void *)&ENEMY_INFO, NULL);
      body_set_rotation(enemy, -PI / 2); // face down
      vector_t offset = {ENEMY_SPACING.x * x, -ENEMY_SPACING.y * y};
      body_set_centroid(enemy, vec_add(ENEMY_SPAWN_TOP_LEFT, offset));
      body_set_velocity(enemy, ENEMY_VELOCITY);
      scene_add_body(state->scene, enemy);
    }
  }

  return state;
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

  if (state->shoot_cooldown > 0.0) {
    state->shoot_cooldown -= dt;
  } else if (sdl_get_key_pressed(' ')) {
    state->shoot_cooldown = SHOOT_INTERVAL;
    body_t *bullet =
        body_init(shape_rectangle(BULLET_SIZE), BULLET_MASS, color_random());
    body_set_centroid(bullet, body_get_centroid(state->player));
    body_set_velocity(bullet, (vector_t){0.0, BULLET_SPEED});
    size_t num_bodies = scene_bodies(state->scene);
    for (size_t i = 0; i < num_bodies; i++) {
      body_t *body = scene_get_body(state->scene, i);
      if (body_get_info(body) == &ENEMY_INFO) {
        create_destructive_collision(state->scene, bullet, body);
      }
    }
    scene_add_body(state->scene, bullet);
  }

  bool enemies_exist = false;
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    vector_t pos = body_get_centroid(body);

    if (body_get_info(body) == &ENEMY_INFO) {
      // this body is an enemy
      enemies_exist = true;
      if (pos.x < ENEMY_RADIUS || pos.x > SCREEN_SIZE.x - ENEMY_RADIUS) {
        // hit wall
        // reverse direction
        vector_t new_vel = vec_negate(body_get_velocity(body));
        body_set_velocity(body, new_vel);
        // move out of wall
        if (pos.x < ENEMY_RADIUS) {
          pos.x += ENEMY_WALL_DISPLACEMENT;
        } else {
          pos.x -= ENEMY_WALL_DISPLACEMENT;
        }
        // move down
        pos.y += ENEMY_ROWS * -ENEMY_SPACING.y;
        body_set_centroid(body, pos);
      }
      if (pos.y < 0) {
        gameover();
      }
      if ((double)rand() / (double)RAND_MAX < ENEMY_BULLETS_PER_SECOND * dt) {
        // enemy shoots
        body_t *bullet = body_init(shape_rectangle(BULLET_SIZE), BULLET_MASS,
                                   color_random());
        body_set_centroid(bullet, body_get_centroid(body));
        body_set_velocity(bullet, (vector_t){0.0, -BULLET_SPEED});
        create_destructive_collision(state->scene, bullet, state->player);
        scene_add_body(state->scene, bullet);
      }
    } else {
      // wrap around
      pos.x = fmod((pos.x + SCREEN_SIZE.x), SCREEN_SIZE.x);
      body_set_centroid(body, pos);

      if (pos.y < 0 || pos.y > SCREEN_SIZE.y) {
        // remove offscreen bullets
        body_remove(body);
      }
    }
  }

  if (!enemies_exist) {
    gameover();
  }

  scene_tick(state->scene, dt);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}