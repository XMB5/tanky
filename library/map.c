#include <map.h>
#include <shape.h>
#include <forces.h>
#include <body.h>
#include <util.h>

static const vector_t SMALL_INTERIOR_WALL_SIZE = {56.0, 224.0};
static const vector_t LARGE_INTERIOR_WALL_SIZE = {56.0, 336.0};
static const double EXTERIOR_WALL_THICKNESS = 100.0;
static const rgb_color_t WALL_COLOR = {1.0, 1.0, 1.0};
static const size_t NUM_BOUNDARIES = 4;
static const size_t NUM_INTERIOR_WALLS = 8;

static const vector_t OBSTACLE_SIZE = {25.0, 25.0};
static const int NUM_OBSTACLES = 10;
static const double OBSTACLE_MASS = 100.0;
static const double OBSTACLE_ELASTICITY = 0.7;

const char *BODY_TYPE_WALL = "wall";
const char *BODY_TYPE_OBSTACLE = "obstacle";

void map_add_walls(scene_t *scene, vector_t screen_size) {
  // Top wall
  vector_t top_wall_centroid = {
      .x = screen_size.x / 2, .y = screen_size.y + EXTERIOR_WALL_THICKNESS / 2};
  body_t *top_wall = body_init_with_info(
      shape_rectangle((vector_t){screen_size.x, EXTERIOR_WALL_THICKNESS}),
      INFINITY, WALL_COLOR, BODY_TYPE_WALL);
  body_set_centroid(top_wall, top_wall_centroid);
  scene_add_body(scene, top_wall);

  // Bottom wall
  vector_t bottom_wall_centroid = {.x = screen_size.x / 2,
                                   .y = -EXTERIOR_WALL_THICKNESS / 2};
  body_t *bottom_wall = body_init_with_info(
      shape_rectangle((vector_t){screen_size.x, EXTERIOR_WALL_THICKNESS}),
      INFINITY, WALL_COLOR, BODY_TYPE_WALL);
  body_set_centroid(bottom_wall, bottom_wall_centroid);
  scene_add_body(scene, bottom_wall);

  // Left wall
  vector_t left_wall_centroid = {.x = -EXTERIOR_WALL_THICKNESS / 2,
                                 .y = screen_size.y / 2};
  body_t *left_wall = body_init_with_info(
      shape_rectangle((vector_t){EXTERIOR_WALL_THICKNESS, screen_size.y}),
      INFINITY, WALL_COLOR, BODY_TYPE_WALL);
  body_set_centroid(left_wall, left_wall_centroid);
  scene_add_body(scene, left_wall);

  // Right wall
  vector_t right_wall_centroid = {
      .x = screen_size.x + EXTERIOR_WALL_THICKNESS / 2, .y = screen_size.y / 2};
  body_t *right_wall = body_init_with_info(
      shape_rectangle((vector_t){EXTERIOR_WALL_THICKNESS, screen_size.y}),
      INFINITY, WALL_COLOR, BODY_TYPE_WALL);
  body_set_centroid(right_wall, right_wall_centroid);
  scene_add_body(scene, right_wall);

  vector_t wall_positions[NUM_INTERIOR_WALLS] = {
      {.x = 150, .y = 100}, {.x = 825, .y = 100}, {.x = 150, .y = 400},
      {.x = 825, .y = 400}, {.x = 350, .y = 250}, {.x = 710, .y = 250},
      {.x = 500, .y = 100}, {.x = 500, .y = 400}};

  vector_t wall_sizes[NUM_INTERIOR_WALLS] = {
      SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE,
      SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE,
      SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE,
      LARGE_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE};

  int rotations[] = {0, 1, 0, 1,
                     0, 1, 0, 1}; // 0 for vertical, 1 for horizontal

  // Generate walls
  for (size_t i = 0; i < NUM_INTERIOR_WALLS; i++) {
    body_t *interior_wall = body_init_with_info(
        shape_rectangle(wall_sizes[i]), INFINITY, WALL_COLOR, BODY_TYPE_WALL);
    if (wall_sizes[i].y == LARGE_INTERIOR_WALL_SIZE.y) {
        body_set_image(interior_wall, "wall_large", 1.0);
    } else {
        body_set_image(interior_wall, "wall_small", 1.0);
    }

    body_set_centroid(interior_wall, wall_positions[i]);

    body_set_rotation(interior_wall, (double)rotations[i] * PI / 2);

    scene_add_body(scene, interior_wall);
  }
}


static bool obstacle_collides (body_t *obstacle, scene_t *scene) {
    size_t num_bodies = scene_bodies(scene);
    for (size_t i = 0; i < num_bodies; i++) {
      body_t *body = scene_get_body(scene, i);
      if (body != obstacle) {
        collision_info_t collision = body_collide(obstacle, body);
        if (collision.collided) {
          return true;
        }
      }
    }
    return false;
}

static void move_obstacle_to_random_point (body_t *obstacle, vector_t screen_size) {
    double x_coord = rand_range(0, screen_size.x);
    double y_coord = rand_range(0, screen_size.y);
    body_set_centroid(obstacle, (vector_t){x_coord, y_coord});
}

void map_init_obstacles(scene_t *scene, vector_t screen_size, size_t num_obstacles) {
    for (size_t i = 0; i < NUM_OBSTACLES; i ++) {
        body_t *obstacle =
        body_init_with_info(shape_rectangle(OBSTACLE_SIZE), OBSTACLE_MASS,
                            COLOR_WHITE, BODY_TYPE_OBSTACLE);
        const char *barricadeImages[] = {"barricadeWood", "barricadeMetal", "crateWood"};
        const char *barricadeImage = barricadeImages[rand() % 3];
        body_set_image(obstacle, barricadeImage, 0.5);
        scene_add_body(scene, obstacle);
        create_drag(scene, 500.0, obstacle);
    }

    map_reset_obstacles(scene, screen_size, num_obstacles);
}

void map_reset_obstacles(scene_t *scene, vector_t screen_size, size_t num_obstacles) {
    size_t num_bodies = scene_bodies(scene);
    for (size_t i = 0; i < num_bodies; i++) {
        body_t *body = scene_get_body(scene, i);
        if (body->type == BODY_TYPE_OBSTACLE) {
            move_obstacle_to_random_point(body, screen_size);
            while (obstacle_collides(body, scene)) {
                move_obstacle_to_random_point(body, screen_size);
            }
        }
    }
}
