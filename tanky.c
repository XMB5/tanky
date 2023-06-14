#include <body.h>
#include <collision.h>
#include <color.h>
#include <font.h>
#include <forces.h>
#include <image.h>
#include <map.h>
#include <math.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <sound.h>
#include <state.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>

static const unsigned int RANDOM_SEED = 12345; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const double BULLET_MASS = .1;
static const double BULLET_ELASTICITY = 1.0;
static const double BULLET_RADIUS = 5.0;
static const double BULLET_OFFSET_RATIO = 1.25;
static const double BULLET_SPEED = 300.0;
static const double BULLET_GRAVITY = 150000.0;
static const char *BODY_TYPE_BULLET = "bullet";
static const char *BODY_TYPE_TANK = "tank";

static const vector_t TANK_SIZE = {40.0, 40.0};
static const double TANK_MASS = 10.0;
static const double TANK_DRAG =
    200.0; // very high drag, so slows down almost instantly
static const double TANK_FORCE = 20000.0;
static const double TANK_ANGULAR_VEL = M_PI;
static const vector_t TANK_IMAGE_OFFSET = (vector_t){0.0, 5.0};

static const vector_t TANK1_INITIAL_POSITION = {80.0, 250.0};
static const vector_t TANK2_INITIAL_POSITION = {920.0, 250.0};

static const double ELASTICITY = 3.0;

static const size_t HEALTH_BAR_MAX_POINTS = 10;
static const double HEALTH_BAR_UNIT_LENGTH = 5.0;
static const double HEALTH_BAR_HEIGHT = 3.0;
static const double HEALTH_BAR_MASS = 1.0;
static const vector_t HEALTH_BAR_TANK_OFFSET = {0.0, 40.0};
static const rgb_color_t HEALTH_BAR_COLOR = {0.0, .76, 0.0};

static const int NUM_OBSTACLES = 10;
static const double OBSTACLE_ELASTICITY = 0.7;
static const double SHOOT_INTERVAL = 1.40; // sec

static const rgb_color_t TEXT_COLOR = {0.392, 0.584, 0.929};

typedef struct tank {
  body_t *body;
  size_t *health;
  body_t *health_bar; // body that represents health bar
  double shot_cooldown;
  bool *was_shot;
  size_t points;
} tank_t;

struct state {
  scene_t *scene;
  tank_t tank_1;
  tank_t tank_2;
};

static void create_tank(state_t *state, tank_t *tank, vector_t pos, char *type){
  tank->body = body_init_with_info(
      shape_rectangle(TANK_SIZE), TANK_MASS, COLOR_WHITE, BODY_TYPE_TANK);
  body_set_centroid(tank->body, pos);
  body_set_image(tank->body, type, .5);
  body_set_image_rotation(tank->body, PI / 2);
  body_set_image_offset(tank->body, TANK_IMAGE_OFFSET);
  create_drag(state->scene, TANK_DRAG, tank->body);
}

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  image_init();
  font_init();
  sound_init();
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();

  // creating the tanks
  create_tank(state, &state->tank_1, TANK1_INITIAL_POSITION, "tank_red");
  create_tank(state, &state->tank_2, TANK2_INITIAL_POSITION, "tank_blue");
  body_set_rotation(state->tank_2.body, PI);
  scene_add_body(state->scene, state->tank_1.body);
  scene_add_body(state->scene, state->tank_2.body);

  // add walls
  map_add_walls(state->scene, SCREEN_SIZE);

  // create health bars
  size_t *health1 = malloc(sizeof(size_t));
  *health1 = HEALTH_BAR_MAX_POINTS;
  state->tank_1.health = health1;
  vector_t health_bar_init_size = {
      HEALTH_BAR_MAX_POINTS * HEALTH_BAR_UNIT_LENGTH, HEALTH_BAR_HEIGHT};
  state->tank_1.health_bar =
      body_init_with_info(shape_rectangle(health_bar_init_size),
                          HEALTH_BAR_MASS, HEALTH_BAR_COLOR, NULL);
  vector_t health_bar_1_init_pos =
      vec_add(TANK1_INITIAL_POSITION, HEALTH_BAR_TANK_OFFSET);
  body_set_centroid(state->tank_1.health_bar, health_bar_1_init_pos);
  scene_add_body(state->scene, state->tank_1.health_bar);

  size_t *health2 = malloc(sizeof(size_t));
  *health2 = HEALTH_BAR_MAX_POINTS;
  state->tank_2.health = health2;
  state->tank_2.health_bar =
      body_init_with_info(shape_rectangle(health_bar_init_size),
                          HEALTH_BAR_MASS, HEALTH_BAR_COLOR, NULL);
  vector_t health_bar_2_init_pos =
      vec_add(TANK2_INITIAL_POSITION, HEALTH_BAR_TANK_OFFSET);
  body_set_centroid(state->tank_2.health_bar, health_bar_2_init_pos);
  scene_add_body(state->scene, state->tank_2.health_bar);

  state->tank_1.was_shot = malloc(sizeof(bool));
  *state->tank_1.was_shot = false;
  state->tank_1.points = 0;

  state->tank_2.was_shot = malloc(sizeof(bool));
  *state->tank_2.was_shot = false;
  state->tank_2.points = 0;

  // add obstacles
  map_init_obstacles(state->scene, SCREEN_SIZE, NUM_OBSTACLES);

  // add collisions
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body_1 = scene_get_body(state->scene, i);
    bool body_1_is_tank = body_1->type == BODY_TYPE_TANK;
    bool body_1_is_wall = body_1->type == BODY_TYPE_WALL;
    bool body_1_is_obstacle = body_1->type == BODY_TYPE_OBSTACLE;

    for (size_t j = i + 1; j < num_bodies; j++) {
      body_t *body_2 = scene_get_body(state->scene, j);
      bool body_2_is_tank = body_2->type == BODY_TYPE_TANK;
      bool body_2_is_wall = body_2->type == BODY_TYPE_WALL;
      bool body_2_is_obstacle = body_2->type == BODY_TYPE_OBSTACLE;

      // Tank x tank
      if ((body_1_is_tank && body_2_is_tank)) {
        create_physics_collision(state->scene, ELASTICITY, body_1, body_2);
      }

      // Tank x wall
      if ((body_1_is_tank && body_2_is_wall) ||
          (body_1_is_wall && body_2_is_tank)) {
        create_physics_collision(state->scene, ELASTICITY, body_1, body_2);
      }

      // Tank x obstacle
      if ((body_1_is_tank && body_2_is_obstacle) ||
          (body_1_is_obstacle && body_2_is_tank)) {
        create_physics_collision(state->scene, OBSTACLE_ELASTICITY, body_1,
                                 body_2);
      }

      // Obstacle x wall
      if ((body_1_is_obstacle && body_2_is_wall) ||
          (body_1_is_wall && body_2_is_obstacle)) {
        create_physics_collision(state->scene, OBSTACLE_ELASTICITY, body_1,
                                 body_2);
      }
    }
  }

  return state;
}

static void update_health_bar(state_t *state, tank_t *tank) {
  vector_t health_bar_size = {*tank->health * HEALTH_BAR_UNIT_LENGTH,
                              HEALTH_BAR_HEIGHT};
  if (tank == &state->tank_1) {
    body_remove(state->tank_1.health_bar);
    state->tank_1.health_bar =
        body_init_with_info(shape_rectangle(health_bar_size), HEALTH_BAR_MASS,
                            HEALTH_BAR_COLOR, NULL);
    scene_add_body(state->scene, state->tank_1.health_bar);
  } else if (tank == &state->tank_2) {
    body_remove(state->tank_2.health_bar);
    state->tank_2.health_bar =
        body_init_with_info(shape_rectangle(health_bar_size), HEALTH_BAR_MASS,
                            HEALTH_BAR_COLOR, NULL);
    scene_add_body(state->scene, state->tank_2.health_bar);
  }
}

static void shoot_bullet(state_t *state, tank_t *tank) {
  sound_play("minigun");
  body_t *bullet =
      body_init_with_info(shape_circle_create(BULLET_RADIUS), BULLET_MASS,
                          COLOR_WHITE, BODY_TYPE_BULLET);
  double angle = body_get_angle(tank->body);
  double bullet_offset = TANK_SIZE.y * BULLET_OFFSET_RATIO / 2;
  bullet->info = malloc_safe(sizeof(size_t));
  bullet->freer = free;
  *(size_t*) bullet->info = 0;
  double bullet_x =
      body_get_centroid(tank->body).x + (cos(angle) * bullet_offset);
  double bullet_y =
      body_get_centroid(tank->body).y + (sin(angle) * bullet_offset);
  body_set_centroid(bullet, (vector_t){bullet_x, bullet_y});
  body_set_velocity(
      bullet, (vector_t){BULLET_SPEED * cos(angle), BULLET_SPEED * sin(angle)});
  

  tank->shot_cooldown = SHOOT_INTERVAL;
  if (tank == &state->tank_1) {
    create_bullet_tank_collision(state->scene, state->tank_2.body, bullet,
                                 state->tank_2.health, state->tank_2.was_shot);
    create_newtonian_gravity(state->scene, BULLET_GRAVITY, state->tank_2.body,
                             bullet);
    body_set_image(bullet, "barrelBlack_top", .28); // actually red
  } else if (tank == &state->tank_2) {
    create_bullet_tank_collision(state->scene, state->tank_1.body, bullet,
                                 state->tank_1.health, state->tank_1.was_shot);
    create_newtonian_gravity(state->scene, BULLET_GRAVITY, state->tank_1.body,
                             bullet);
    body_set_image(bullet, "barrelBlue_top", .28);
  }

  // wall and obstacle collisions
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (body->type == BODY_TYPE_WALL) {
      create_bullet_wall_collision(state->scene, BULLET_ELASTICITY, bullet, body);
    }
    if (body->type == BODY_TYPE_OBSTACLE) {
      create_bullet_obstacle_collision(state->scene, body, bullet);
    }
  }
  scene_add_body(state->scene, bullet);

}

static void clear_bullets(state_t *state) {
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (body->type == BODY_TYPE_BULLET) {
      body_remove(body);
    }
  }
}

static void tank_dead(state_t *state, tank_t *tank) {
  *tank->health = HEALTH_BAR_MAX_POINTS;
  update_health_bar(state, tank);
  if (tank == &state->tank_1) {
    state->tank_2.points = state->tank_2.points + 1;
    body_set_centroid(state->tank_1.body, TANK1_INITIAL_POSITION);
    body_set_rotation(state->tank_1.body, 0);
  } else if (tank == &state->tank_2) {
    state->tank_1.points = state->tank_1.points + 1;
    body_set_centroid(state->tank_2.body, TANK2_INITIAL_POSITION);
    body_set_rotation(state->tank_2.body, PI);
  }
  clear_bullets(state);
}

static void reset (state_t *state) {
  map_reset_obstacles(state->scene, SCREEN_SIZE, NUM_OBSTACLES);

  // Reset players and bullets
  tank_dead(state, &state->tank_1);
  tank_dead(state, &state->tank_2);

  state->tank_1.points = 0;
  state->tank_2.points = 0;
}

/**
 * rotate, but don't allow rotating into a wall (would cause glitches)
*/
static void tank_rotate(state_t *state, tank_t *tank, double angular_vel, double dt) {
  double dtheta = angular_vel * dt;
  body_set_rotation(tank->body, tank->body->angle + dtheta);
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (body->type == BODY_TYPE_WALL) {
      collision_info_t collision = find_collision(body->shape, tank->body->shape);
      if (collision.collided) {
        body_set_rotation(tank->body, tank->body->angle - dtheta);
        break;
      }
    }
  }
}

static void healthbar_update(state_t *state, tank_t *tank) {
  vector_t tank_coords = body_get_centroid(tank->body);
  body_set_centroid(tank->health_bar, vec_add(tank_coords, HEALTH_BAR_TANK_OFFSET));

  if (*tank->was_shot) {
    update_health_bar(state, tank);
    *tank->was_shot = false;
  }

  if (*tank->health == 0 || *tank->health > HEALTH_BAR_MAX_POINTS) {
    tank_dead(state, tank);
  }
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  // draw background image
  image_t *tile = image_load("tileSand1_big"); // loads from cache
  vector_t pos = vec_multiply(0.5, SCREEN_SIZE);
  double scale = 1.0;
  double rot = 0.0;
  scene_draw_image(state->scene, tile, pos, scale, rot);

  // tank forward/backward movement
  if (sdl_get_key_pressed(UP_ARROW)) {
    vector_t force = vec_rotate((vector_t){TANK_FORCE, 0.0},
                                body_get_angle(state->tank_2.body));
    body_add_force(state->tank_2.body, force);
  } else if (sdl_get_key_pressed(DOWN_ARROW)) {
    vector_t force = vec_rotate((vector_t){-TANK_FORCE, 0.0},
                                body_get_angle(state->tank_2.body));
    body_add_force(state->tank_2.body, force);
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

  // tank rotating
  if (sdl_get_key_pressed('d')) {
    tank_rotate(state, &state->tank_1, -TANK_ANGULAR_VEL, dt);
  } else if (sdl_get_key_pressed('a')) {
    tank_rotate(state, &state->tank_1, TANK_ANGULAR_VEL, dt);
  }

  if (sdl_get_key_pressed(RIGHT_ARROW)) {
    tank_rotate(state, &state->tank_2, -TANK_ANGULAR_VEL, dt);
  } else if (sdl_get_key_pressed(LEFT_ARROW)) {
    tank_rotate(state, &state->tank_2, TANK_ANGULAR_VEL, dt);
  }

  // bullet shooting
  state->tank_1.shot_cooldown -= dt;
  if (sdl_get_key_pressed('e') && state->tank_1.shot_cooldown <= 0) {
    shoot_bullet(state, &state->tank_1);
  }
  state->tank_2.shot_cooldown -= dt;
  if (sdl_get_key_pressed('/') && state->tank_2.shot_cooldown <= 0) {
    shoot_bullet(state, &state->tank_2);
  }

  // update health bars (includes tank death handling)
  healthbar_update(state, &state->tank_1);
  healthbar_update(state, &state->tank_2);

  // points text display
  const size_t MAX_STR_SIZE = 256;
  char points_str[MAX_STR_SIZE];
  snprintf(points_str, MAX_STR_SIZE, "Red Tank Points: %zu",
           state->tank_1.points);
  vector_t text_top_left = {200, 480};
  scene_draw_text(state->scene, points_str, text_top_left, TEXT_COLOR);

  snprintf(points_str, MAX_STR_SIZE, "Blue Tank Points: %zu",
           state->tank_2.points);
  vector_t text_top_right = {600, 480};
  scene_draw_text(state->scene, points_str, text_top_right, TEXT_COLOR);

  // reset button
  static bool just_reset = false;
  if (sdl_get_key_pressed('u')) {
    if(!just_reset){
      reset(state);
      just_reset = true;
    }
  } else {
      just_reset = false;
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
  sound_deinit();
}
