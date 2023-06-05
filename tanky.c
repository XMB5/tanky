#include <body.h>
#include <collision.h>
#include <color.h>
#include <font.h>
#include <forces.h>
#include <image.h>
#include <math.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>

static const unsigned int RANDOM_SEED = 12346; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const double EXTERIOR_WALL_THICKNESS = 100.0;

static const vector_t BULLET_SIZE = {20.0, 10.0};
static const double BULLET_MASS = 1.0;
static const double BULLET_RADIUS = 5.0;
static const vector_t BULLET_VELOCITY = {0.0, 200.0};
static const vector_t BULLET_INITIAL_VEL = {250.0, -400.0};
static const rgb_color_t BULLET_COLOR = {1.0, 1.0, 0.0};
static const uint8_t BULLET_INFO =
    0; // address of BULLET_INFO specifies body is a bullet

static const vector_t TANK_SIZE = {40.0, 30.0};
static const double TANK_MASS = 1.0;
static const double TANK_DRAG =
    20.0; // very high drag, so slows down almost instantly
static const double TANK_FORCE = 2000.0;
static const double TANK_ANGULAR_VEL = M_PI;
static const vector_t TANK_IMAGE_OFFSET = (vector_t){0.0, 5.0};

static const vector_t TANK1_INITIAL_POSITION = {400.0, 300.0};
static const uint8_t TANK1_INFO =
    0; // address of TANK1_INFO specifies body is TANK1

static const vector_t TANK2_INITIAL_POSITION = {800.0, 300.0};
static const uint8_t TANK2_INFO =
    0; // address of TANK2_INFO specifies body is TANK2

static const double ELASTICITY = 7.5;

static const size_t POINTS_PER_BULLET = 1;

static const size_t HEALTH_BAR_MAX_POINTS = 10;
static const double HEALTH_BAR_UNIT_LENGTH = 5.0;
static const double HEALTH_BAR_HEIGHT = 1.0;
static const double HEALTH_BAR_MASS = 1.0;
static const vector_t HEALTH_BAR_TANK_OFFSET = {0.0, 40.0};
static const rgb_color_t HEALTH_BAR_COLOR = {0.0, 1.0, 0.0};
static const uint8_t HEALTH_BAR_1_INFO = 0;
static const uint8_t HEALTH_BAR_2_INFO = 0;

typedef struct tank {
  body_t *body;
  size_t *health;
  body_t *health_bar; // body that represents health bar
  list_t *powerups;
  double shot_cooldown;
  bool *was_shot;
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

  state->tank_1.body =
      body_init_with_info(shape_rectangle(TANK_SIZE), TANK_MASS, COLOR_WHITE,
                          (void *)&TANK1_INFO, NULL);
  state->tank_2.body =
      body_init_with_info(shape_rectangle(TANK_SIZE), TANK_MASS, COLOR_WHITE,
                          (void *)&TANK2_INFO, NULL);
  body_set_centroid(state->tank_1.body, TANK1_INITIAL_POSITION);
  body_set_centroid(state->tank_2.body, TANK2_INITIAL_POSITION);
  scene_add_body(state->scene, state->tank_1.body);
  scene_add_body(state->scene, state->tank_2.body);
  body_set_image(state->tank_1.body, "tank_red", .5);
  body_set_image(state->tank_2.body, "tank_blue", .5);
  body_set_image_rotation(state->tank_1.body, PI / 2);
  body_set_image_rotation(state->tank_2.body, 3 * PI / 2);
  body_set_image_offset(state->tank_1.body, TANK_IMAGE_OFFSET);
  body_set_image_offset(state->tank_2.body, TANK_IMAGE_OFFSET);
  create_drag(state->scene, TANK_DRAG, state->tank_1.body);
  create_drag(state->scene, TANK_DRAG, state->tank_2.body);

  // create health bars
  size_t *health1 = malloc(sizeof(size_t));  // needs to be freed at some point
  *health1 = HEALTH_BAR_MAX_POINTS;
  state->tank_1.health = health1;
  vector_t health_bar_init_size = {
      HEALTH_BAR_MAX_POINTS * HEALTH_BAR_UNIT_LENGTH, HEALTH_BAR_HEIGHT};
  state->tank_1.health_bar = body_init_with_info(
      shape_rectangle(health_bar_init_size), HEALTH_BAR_MASS, HEALTH_BAR_COLOR,
      (void *)&HEALTH_BAR_1_INFO, NULL);
  vector_t health_bar_1_init_pos =
      vec_add(TANK1_INITIAL_POSITION, HEALTH_BAR_TANK_OFFSET);
  body_set_centroid(state->tank_1.health_bar, health_bar_1_init_pos);
  scene_add_body(state->scene, state->tank_1.health_bar);

  size_t *health2 = malloc(sizeof(size_t));
  *health2 = HEALTH_BAR_MAX_POINTS;
  state->tank_2.health = health2;
  state->tank_2.health_bar = body_init_with_info(
      shape_rectangle(health_bar_init_size), HEALTH_BAR_MASS, HEALTH_BAR_COLOR,
      (void *)&HEALTH_BAR_2_INFO, NULL);
  vector_t health_bar_2_init_pos =
      vec_add(TANK2_INITIAL_POSITION, HEALTH_BAR_TANK_OFFSET);
  body_set_centroid(state->tank_2.health_bar, health_bar_2_init_pos);
  scene_add_body(state->scene, state->tank_2.health_bar);

  state->tank_1.was_shot = malloc(sizeof(bool));
  *state->tank_1.was_shot = false;

  state->tank_2.was_shot = malloc(sizeof(bool));
  *state->tank_2.was_shot = false;


  // collisions
  // create_physics_collision(state->scene, 0.0, state->map->walls,
  // state->tank_1.body); create_physics_collision(state->scene, 0.0,
  // state->map->walls, state->tank_2.body);
  // create_physics_collision(state->scene, ELASTICITY, state->map->obstacles,
  // state->tank_1.body); create_physics_collision(state->scene, ELASTICITY,
  // state->map->obstacles, state->tank_2.body);
  create_physics_collision(state->scene, ELASTICITY, state->tank_1.body,
                           state->tank_2.body);

  return state;
}

static void update_health_bar(state_t *state, tank_t tank) {
  vector_t health_bar_size = {*tank.health * HEALTH_BAR_UNIT_LENGTH,
                              HEALTH_BAR_HEIGHT};
  if (body_get_info(tank.body) == &TANK1_INFO) {
    body_remove(state->tank_1.health_bar);
    state->tank_1.health_bar =
        body_init_with_info(shape_rectangle(health_bar_size), HEALTH_BAR_MASS,
                            HEALTH_BAR_COLOR, (void *)&HEALTH_BAR_1_INFO, NULL);
    scene_add_body(state->scene, state->tank_1.health_bar);
  } else if (body_get_info(tank.body) == &TANK2_INFO) {
    body_remove(state->tank_2.health_bar);
    state->tank_2.health_bar =
        body_init_with_info(shape_rectangle(health_bar_size), HEALTH_BAR_MASS,
                            HEALTH_BAR_COLOR, (void *)&HEALTH_BAR_2_INFO, NULL);
    scene_add_body(state->scene, state->tank_2.health_bar);
  }
}

static void shoot_bullet(state_t *state, tank_t *tank) {
  body_t *bullet =
      body_init_with_info(shape_circle_create(BULLET_RADIUS), BULLET_MASS,
                          BULLET_COLOR, (void *)&BULLET_INFO, NULL);
  body_set_rotation(bullet, body_get_angle(tank->body));
  body_set_centroid(
      bullet, (vector_t){body_get_centroid(tank->body).x, body_get_centroid(tank->body).y});
  body_set_velocity(bullet, BULLET_VELOCITY);

  if (body_get_info(tank->body) == &TANK1_INFO) {
    // create_destructive_collision(state->scene, state->tank_2.body, bullet);
    create_bullet_collision(state->scene, state->tank_2.body, bullet, state->tank_2.health, state->tank_2.was_shot);
  }
  else if (body_get_info(tank->body) == &TANK2_INFO) {
    // create_destructive_collision(state->scene, state->tank_1.body, bullet);
    create_bullet_collision(state->scene, state->tank_1.body, bullet, state->tank_1.health, state->tank_1.was_shot);
  }

  // size_t num_bodies = scene_bodies(state->scene);
  // for (size_t i = 0; i < num_bodies; i++) {
  //   body_t *body = scene_get_body(state->scene, i);
  //   if (body_get_info(body) == &TANK1_INFO) {
  //     create_destructive_collision(state->scene, body, bullet);
  //   } else if (body_get_info(body) == &TANK2_INFO) {
  //     create_destructive_collision(state->scene, body, bullet);
  //   }
  // }
  scene_add_body(state->scene, bullet);
}

static void tank_dead(state_t *state, tank_t tank) {
  *tank.health = HEALTH_BAR_MAX_POINTS;
  if (body_get_info(tank.body) == &TANK1_INFO) {
    state->tank_2.points = state->tank_2.points + 1;
  }
  else if (body_get_info(tank.body) == &TANK2_INFO) {
    state->tank_1.points = state->tank_1.points + 1;
  }
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  if (sdl_get_key_pressed(UP_ARROW)) {
    vector_t force = vec_rotate((vector_t){-TANK_FORCE, 0.0},
                                body_get_angle(state->tank_2.body));
    body_add_force(state->tank_2.body, force);
  } else if (sdl_get_key_pressed(DOWN_ARROW)) {
    vector_t force = vec_rotate((vector_t){TANK_FORCE, 0.0},
                                body_get_angle(state->tank_2.body));
    body_add_force(state->tank_2.body, force);
  }

  if (sdl_get_key_pressed(RIGHT_ARROW)) {
    body_set_angular_velocity(state->tank_2.body, -TANK_ANGULAR_VEL);
  } else if (sdl_get_key_pressed(LEFT_ARROW)) {
    body_set_angular_velocity(state->tank_2.body, TANK_ANGULAR_VEL);
  } else {
    body_set_angular_velocity(state->tank_2.body, 0.0);
  }

  // bullet shooting
  if (sdl_get_key_pressed('/')) {
    shoot_bullet(state, &state->tank_2);
  }
  if (sdl_get_key_pressed('e')) {
    shoot_bullet(state, &state->tank_1);
  }

  if (sdl_get_key_pressed('s')) {
    vector_t force = vec_rotate((vector_t){-TANK_FORCE, 0.0},
                                body_get_angle(state->tank_1.body));
    body_add_force(state->tank_1.body, force);
  } else if (sdl_get_key_pressed('w')) {
    vector_t force = vec_rotate((vector_t){TANK_FORCE, 0.0},
                                body_get_angle(state->tank_1.body));
    body_add_force(state->tank_1.body, force);
  }

  if (sdl_get_key_pressed('d')) {
    body_set_angular_velocity(state->tank_1.body, -TANK_ANGULAR_VEL);
  } else if (sdl_get_key_pressed('a')) {
    body_set_angular_velocity(state->tank_1.body, TANK_ANGULAR_VEL);
  } else {
    body_set_angular_velocity(state->tank_1.body, 0.0);
  }

  const size_t MAX_STR_SIZE = 256;
  char display_str[MAX_STR_SIZE];
  vector_t tank_1_coords = body_get_centroid(state->tank_1.body);
  vector_t tank_2_coords = body_get_centroid(state->tank_2.body);
  snprintf(display_str, MAX_STR_SIZE, "tank coords: %f,%f\t blue tank points: %zu\t red tank points: %zu\t blue tank shot: %d", tank_1_coords.x,
           tank_1_coords.y, state->tank_2.points, state->tank_1.points, *state->tank_2.was_shot);
  vector_t text_top_left = {20, 480};
  scene_draw_text(state->scene, display_str, text_top_left, COLOR_WHITE);

  body_set_centroid(state->tank_1.health_bar,
                    vec_add(tank_1_coords, HEALTH_BAR_TANK_OFFSET));
  body_set_centroid(state->tank_2.health_bar,
                    vec_add(tank_2_coords, HEALTH_BAR_TANK_OFFSET));

  if (*state->tank_1.was_shot) {
    update_health_bar(state, state->tank_1);
    *state->tank_1.was_shot = false;
  }
  if (*state->tank_2.was_shot) {
    update_health_bar(state, state->tank_2);
    *state->tank_2.was_shot = false;
  }

  if (*state->tank_1.health == 0 || *state->tank_1.health > HEALTH_BAR_MAX_POINTS) {
    tank_dead(state, state->tank_1);
  }
  if (*state->tank_2.health == 0 || *state->tank_2.health > HEALTH_BAR_MAX_POINTS) {
    tank_dead(state, state->tank_2);
  }

  scene_tick(state->scene, dt);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  free(state->tank_1.health);
  free(state->tank_2.health);
  free(state->tank_1.was_shot);
  free(state->tank_2.was_shot);
  scene_free(state->scene);
  free(state);
  image_deinit();
  font_deinit();
}