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
#include <sound.h>
#include <state.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>

static const unsigned int RANDOM_SEED = 12345; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const vector_t INTERIOR_WALL_SIZE = {50.0, 200.0};
static const double EXTERIOR_WALL_THICKNESS = 100.0;
static const rgb_color_t WALL_COLOR = {1.0, 1.0, 1.0};
static const size_t NUM_BOUNDARIES = 4;
static const size_t NUM_INTERIOR_WALLS = 9;
static const uint8_t WALL_INFO =
    0; // address of WALL_INFO specifies body is a wall

static const vector_t BULLET_SIZE = {10.0, 5.0};
static const double BULLET_MASS = .1;
static const double BULLET_ELASTICITY = 1.0;
static const double BULLET_RADIUS = 5.0;
static const double BULLET_OFFSET_RATIO = 1.25;
static const double BULLET_SPEED = -300.0;
static const double BULLET_GRAVITY = 100000.0;
static const vector_t BULLET_INITIAL_VEL = {250.0, -400.0};
static const vector_t BULLET_IMAGE_OFFSET = (vector_t){0.0, 5.0};
static const uint8_t BULLET_INFO =
    0; // address of BULLET_INFO specifies body is a bullet

static const vector_t TANK_SIZE = {40.0, 30.0};
static const double TANK_MASS = 10.0;
static const double TANK_DRAG =
    200.0; // very high drag, so slows down almost instantly
static const double TANK_FORCE = 20000.0;
static const double TANK_ANGULAR_VEL = M_PI;
static const vector_t TANK_IMAGE_OFFSET = (vector_t){0.0, 5.0};

static const vector_t TANK1_INITIAL_POSITION = {80.0, 250.0};
static const uint8_t TANK1_INFO =
    0; // address of TANK1_INFO specifies body is TANK1

static const vector_t TANK2_INITIAL_POSITION = {920.0, 250.0};
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

static const vector_t OBSTACLE_SIZE = {25.0, 25.0};
static const int NUM_OBSTACLES = 10;
static const double OBSTACLE_MASS = 100.0;
static const double OBSTACLE_ELASTICITY = 0.01;
static const uint8_t OBSTACLE_INFO =
    0; // address of OBSTACLE_INFO specifies body is an obstacle

static const double SHOOT_INTERVAL = 1.5; // sec

static const rgb_color_t TEXT_COLOR = {0.392, 0.584, 0.929};

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
  double shoot_cooldown_pl1;
  double shoot_cooldown_pl2;
};

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  image_init();
  font_init();
  sound_init();
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();

  list_t *walls = list_init(NUM_BOUNDARIES + NUM_INTERIOR_WALLS, free);

  // TODO: create walls
  // Top wall
  vector_t top_wall_centroid = {
      .x = SCREEN_SIZE.x / 2, .y = SCREEN_SIZE.y + EXTERIOR_WALL_THICKNESS / 2};
  body_t *top_wall = body_init_with_info(
      shape_rectangle((vector_t){SCREEN_SIZE.x, EXTERIOR_WALL_THICKNESS}),
      INFINITY, WALL_COLOR, (void *)&WALL_INFO, NULL);
  body_set_centroid(top_wall, top_wall_centroid);
  list_add(walls, top_wall);

  // Bottom wall
  vector_t bottom_wall_centroid = {.x = SCREEN_SIZE.x / 2,
                                   .y = -EXTERIOR_WALL_THICKNESS / 2};
  body_t *bottom_wall = body_init_with_info(
      shape_rectangle((vector_t){SCREEN_SIZE.x, EXTERIOR_WALL_THICKNESS}),
      INFINITY, WALL_COLOR, (void *)&WALL_INFO, NULL);
  body_set_centroid(bottom_wall, bottom_wall_centroid);
  list_add(walls, bottom_wall);

  // Left wall
  vector_t left_wall_centroid = {.x = -EXTERIOR_WALL_THICKNESS / 2,
                                 .y = SCREEN_SIZE.y / 2};
  body_t *left_wall = body_init_with_info(
      shape_rectangle((vector_t){EXTERIOR_WALL_THICKNESS, SCREEN_SIZE.y}),
      INFINITY, WALL_COLOR, (void *)&WALL_INFO, NULL);
  body_set_centroid(left_wall, left_wall_centroid);
  list_add(walls, left_wall);

  // Right wall
  vector_t right_wall_centroid = {
      .x = SCREEN_SIZE.x + EXTERIOR_WALL_THICKNESS / 2, .y = SCREEN_SIZE.y / 2};
  body_t *right_wall = body_init_with_info(
      shape_rectangle((vector_t){EXTERIOR_WALL_THICKNESS, SCREEN_SIZE.y}),
      INFINITY, WALL_COLOR, (void *)&WALL_INFO, NULL);
  body_set_centroid(right_wall, right_wall_centroid);
  list_add(walls, right_wall);

  vector_t wall_positions[NUM_INTERIOR_WALLS] = {
      {.x = 150, .y = 100}, {.x = 850, .y = 100}, {.x = 150, .y = 400},
      {.x = 850, .y = 400}, {.x = 350, .y = 250}, {.x = 650, .y = 250},
      {.x = 500, .y = 100}, {.x = 500, .y = 400}, {.x = 750, .y = 250}};

  int rotations[] = {0, 1, 0, 1, 0,
                     1, 0, 1, 1}; // 0 for vertical, 1 for horizontal

  // Generate walls
  for (size_t i = 0; i < NUM_INTERIOR_WALLS; i++) {
    body_t *interior_wall =
        body_init_with_info(shape_rectangle(INTERIOR_WALL_SIZE), INFINITY,
                            WALL_COLOR, (void *)&WALL_INFO, NULL);

    body_set_centroid(interior_wall, wall_positions[i]);

    body_set_rotation(interior_wall, (double)rotations[i] * PI / 2);

    list_add(walls, interior_wall);
  }

  state->map.walls = walls;

  // create tanks
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

  // Set collisions
  for (size_t i = 0; i < NUM_BOUNDARIES + NUM_INTERIOR_WALLS; i++) {
    scene_add_body(state->scene, list_get(walls, i));
  }

  // create health bars
  size_t *health1 = malloc(sizeof(size_t)); // needs to be freed at some point
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

  // create obstacles
  size_t num_obstacles_created = 0;
  list_t *obstacles = list_init(NUM_OBSTACLES, free);
  while (num_obstacles_created < NUM_OBSTACLES) {
    double x_coord = rand_range(0, SCREEN_SIZE.x);
    double y_coord = rand_range(0, SCREEN_SIZE.y);
    body_t *obstacle =
        body_init_with_info(shape_rectangle(OBSTACLE_SIZE), OBSTACLE_MASS,
                            COLOR_WHITE, (void *)&OBSTACLE_INFO, NULL);
    body_set_centroid(obstacle, (vector_t){x_coord, y_coord});
    list_add(obstacles, obstacle);
    scene_add_body(state->scene, obstacle);

    size_t num_bodies = scene_bodies(state->scene);

    bool obstacle_collides = false;
    for (size_t i = 0; i < num_bodies; i++) {
      body_t *body = scene_get_body(state->scene, i);
      if (body != obstacle) {
        collision_info_t collision = body_collide(obstacle, body);
        if (collision.collided) {
          obstacle_collides = true;
          break;
        }
      }
    }

    if (obstacle_collides == true) {
      scene_remove_body(state->scene,
                        num_bodies -
                            1); // Remove newly placed obstacle, and try again
    } else {
      num_obstacles_created++;
    }
  }

  size_t num_bodies = scene_bodies(state->scene);

  for (size_t i = 0; i < num_bodies; i++) {
    body_t* body_1 = scene_get_body(state->scene, i);
    bool body_1_is_tank = (body_get_info(body_1) == &TANK1_INFO) || (body_get_info(body_1) == &TANK2_INFO);
    bool body_1_is_wall = (body_get_info(body_1) == &WALL_INFO);
    bool body_1_is_obstacle = (body_get_info(body_1) == &OBSTACLE_INFO);
    
    for (size_t j = i+1; j < num_bodies; j++) {
        body_t* body_2 = scene_get_body(state->scene, j);
        bool body_2_is_tank = (body_get_info(body_2) == &TANK1_INFO) || (body_get_info(body_2) == &TANK2_INFO);
        bool body_2_is_wall = (body_get_info(body_2) == &WALL_INFO);
        bool body_2_is_obstacle = (body_get_info(body_2) == &OBSTACLE_INFO);

        // Tank x tank
        if ((body_1_is_tank && body_2_is_tank)) {
          create_physics_collision(state->scene, ELASTICITY, body_1, body_2);
        }

        // Tank x wall
        if ((body_1_is_tank && body_2_is_wall) || (body_1_is_wall && body_2_is_tank)) {
          create_physics_collision(state->scene, ELASTICITY, body_1, body_2);
        }

        // Tank x obstacle
        if ((body_1_is_tank && body_2_is_obstacle) || (body_1_is_obstacle && body_2_is_tank)) {
          create_physics_collision(state->scene, ELASTICITY, body_1, body_2);
        }

        // Obstacle x wall
        if ((body_1_is_obstacle && body_2_is_wall) || (body_1_is_wall && body_2_is_obstacle)) {
          create_physics_collision(state->scene, ELASTICITY, body_1, body_2);
        }

    }
  }

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
  sound_play("minigun");
  body_t *bullet =
      body_init_with_info(shape_circle_create(BULLET_RADIUS), BULLET_MASS,
                          COLOR_WHITE, (void *)&BULLET_INFO, NULL);
  double angle = body_get_angle(tank->body);
  double bullet_offset = TANK_SIZE.y * BULLET_OFFSET_RATIO / 2;
  if (body_get_info(tank->body) == &TANK1_INFO) {
    angle += PI;
  }
  double bullet_x =
      body_get_centroid(tank->body).x - (cos(angle) * bullet_offset);
  double bullet_y =
      body_get_centroid(tank->body).y - (sin(angle) * bullet_offset);
  body_set_centroid(bullet, (vector_t){bullet_x, bullet_y});
  body_set_velocity(
      bullet, (vector_t){BULLET_SPEED * cos(angle), BULLET_SPEED * sin(angle)});

  if (body_get_info(tank->body) == &TANK1_INFO) {
    state->shoot_cooldown_pl1 = SHOOT_INTERVAL;
    create_bullet_collision(state->scene, state->tank_2.body, bullet,
                            state->tank_2.health, state->tank_2.was_shot);
    create_newtonian_gravity(state->scene, BULLET_GRAVITY, state->tank_2.body,
                             bullet);
  } else if (body_get_info(tank->body) == &TANK2_INFO) {
    state->shoot_cooldown_pl2 = SHOOT_INTERVAL;
    create_bullet_collision(state->scene, state->tank_1.body, bullet,
                            state->tank_1.health, state->tank_1.was_shot);
    create_newtonian_gravity(state->scene, BULLET_GRAVITY, state->tank_1.body,
                             bullet);
  }

  // wall collisions
  size_t num_bodies = scene_bodies(state->scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (body_get_info(body) == &WALL_INFO) {
      create_physics_collision(state->scene, BULLET_ELASTICITY, body, bullet);
    }
  }
  scene_add_body(state->scene, bullet);

  // Overlaying the bullet with an image
  // body_set_image(scene->bullet, "bulletDark1_outline", .1);
  // body_set_image_rotation(scene->bullet, angle);
  // body_set_image_offset(scene->bullet, BULLET_IMAGE_OFFSET);
}

static void tank_dead(state_t *state, tank_t tank) {
  *tank.health = HEALTH_BAR_MAX_POINTS;
  update_health_bar(state, tank);
  if (body_get_info(tank.body) == &TANK1_INFO) {
    state->tank_2.points = state->tank_2.points + 1;
    body_set_centroid(state->tank_1.body, TANK1_INITIAL_POSITION);
    // TODO: CLEAR BULLETS
  } else if (body_get_info(tank.body) == &TANK2_INFO) {
    state->tank_1.points = state->tank_1.points + 1;
    body_set_centroid(state->tank_2.body, TANK2_INITIAL_POSITION);
    // TODO: CLEAR BULLETS
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
  if (state->shoot_cooldown_pl2 > 0.0) {
    state->shoot_cooldown_pl2 -= dt;
  }
  if (sdl_get_key_pressed('/')) {
    if (state->shoot_cooldown_pl2 <= 0) {
      shoot_bullet(state, &state->tank_2);
    }
  }
  if (state->shoot_cooldown_pl1 > 0.0) {
    state->shoot_cooldown_pl1 -= dt;
  }
  if (sdl_get_key_pressed('e')) {
    if (state->shoot_cooldown_pl1 <= 0) {
      shoot_bullet(state, &state->tank_1);
    }
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
  // char display_str[MAX_STR_SIZE];
  vector_t tank_1_coords = body_get_centroid(state->tank_1.body);
  vector_t tank_2_coords = body_get_centroid(state->tank_2.body);
  // snprintf(display_str, MAX_STR_SIZE,
  //          "tank coords: %f,%f\t blue tank points: %zu\t red tank points: "
  //          "%zu\t",
  //          tank_1_coords.x, tank_1_coords.y, state->tank_2.points,
  //          state->tank_1.points);
  // vector_t text_top_left = {20, 480};
  // scene_draw_text(state->scene, display_str, text_top_left, COLOR_WHITE);

  char tank_1_points_str[MAX_STR_SIZE];
  snprintf(tank_1_points_str, MAX_STR_SIZE,
            "Red Tank Points: %zu", state->tank_1.points);
  vector_t text_top_left = {200, 480};
  scene_draw_text(state->scene, tank_1_points_str, text_top_left, TEXT_COLOR);

  char tank_2_points_str[MAX_STR_SIZE];
  snprintf(tank_2_points_str, MAX_STR_SIZE,
            "Blue Tank Points: %zu", state->tank_2.points);
  vector_t text_top_right = {600, 480};
  scene_draw_text(state->scene, tank_2_points_str, text_top_right, TEXT_COLOR);

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

  if (*state->tank_1.health == 0 ||
      *state->tank_1.health > HEALTH_BAR_MAX_POINTS) {
    tank_dead(state, state->tank_1);
  }
  if (*state->tank_2.health == 0 ||
      *state->tank_2.health > HEALTH_BAR_MAX_POINTS) {
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
  sound_deinit();
}
