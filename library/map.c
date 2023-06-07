#include <map.h>
#include <util.h>
#include <shape.h>

static const vector_t SMALL_INTERIOR_WALL_SIZE = {50.0, 200.0};
static const vector_t LARGE_INTERIOR_WALL_SIZE = {50.0, 300.0};
static const double EXTERIOR_WALL_THICKNESS = 100.0;
static const rgb_color_t WALL_COLOR = {1.0, 1.0, 1.0};
static const size_t NUM_BOUNDARIES = 4;
static const size_t NUM_INTERIOR_WALLS = 9;

const char *BODY_TYPE_WALL = "wall";

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
      {.x = 150, .y = 100}, {.x = 850, .y = 100}, {.x = 150, .y = 400},
      {.x = 850, .y = 400}, {.x = 350, .y = 250}, {.x = 700, .y = 250},
      {.x = 500, .y = 100}, {.x = 500, .y = 400}};

  vector_t wall_sizes[NUM_INTERIOR_WALLS] = {
      SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE,
      SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE,
      LARGE_INTERIOR_WALL_SIZE, SMALL_INTERIOR_WALL_SIZE
  };

  int rotations[] = {0, 1, 0, 1, 0,
                     1, 0, 1}; // 0 for vertical, 1 for horizontal

    // Generate walls
  for (size_t i = 0; i < NUM_INTERIOR_WALLS; i++) {
    body_t *interior_wall =
        body_init_with_info(shape_rectangle(wall_sizes[i]), INFINITY, WALL_COLOR, BODY_TYPE_WALL);

    body_set_centroid(interior_wall, wall_positions[i]);

    body_set_rotation(interior_wall, (double)rotations[i] * PI / 2);

    scene_add_body(scene, interior_wall);
  }
}
