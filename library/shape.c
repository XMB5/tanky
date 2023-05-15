#include <math.h>
#include <polygon.h>
#include <shape.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>

static const size_t CIRCLE_NUM_SIDES = 50;
static const size_t ELLIPSE_NUM_SIDES = 50;
static const double PACMAN_MOUTH_ANGLE = 1.0; // radians

list_t *shape_star_create(size_t star_points, double outer_radius,
                          double inner_radius) {
  size_t num_vertices = star_points * 2;
  list_t *shape = list_init(num_vertices, free);
  double angle = 0.0;
  for (size_t i = 0; i < num_vertices; i++) {
    // alternate outer/inner radius
    double radius = (i % 2 == 0) ? outer_radius : inner_radius;

    vector_t *vertex = malloc_safe(sizeof(vector_t));
    vertex->x = cos(angle) * radius;
    vertex->y = sin(angle) * radius;
    list_add(shape, vertex);

    angle += 2.0 * PI / num_vertices;
  }

  return shape;
}

list_t *shape_pacman_create(double radius) {
  return shape_arc_sweep(radius, PACMAN_MOUTH_ANGLE);
}

list_t *shape_arc_sweep(double radius, double empty_angle) {
  list_t *shape = list_init(CIRCLE_NUM_SIDES - 1 + 1, free);
  for (size_t i = 0; i < CIRCLE_NUM_SIDES - 1; i++) {
    double angle =
        (double)i / (double)(CIRCLE_NUM_SIDES - 1) * (2.0 * PI - empty_angle) +
        empty_angle / 2.0;
    vector_t *vertex = malloc_safe(sizeof(vector_t));
    vertex->x = cos(angle) * radius;
    vertex->y = sin(angle) * radius;
    list_add(shape, vertex);
  }
  vector_t *center = malloc_safe(sizeof(vector_t));
  *center = VEC_ZERO;
  list_add(shape, center);
  return shape;
}

list_t *shape_circle_create(double radius) {
  list_t *shape = list_init(CIRCLE_NUM_SIDES, free);
  for (size_t i = 0; i < CIRCLE_NUM_SIDES; i++) {
    double angle = (double)i / (double)CIRCLE_NUM_SIDES * 2.0 * PI;
    vector_t *vertex = malloc_safe(sizeof(vector_t));
    vertex->x = cos(angle) * radius;
    vertex->y = sin(angle) * radius;
    list_add(shape, vertex);
  }
  return shape;
}

list_t *shape_rectangle(vector_t size) { // rectangle centered on (0,0)
  const size_t NUM_CORNERS = 4;
  list_t *shape = list_init(NUM_CORNERS, free);
  vector_t *corners[NUM_CORNERS];
  for (size_t i = 0; i < NUM_CORNERS; i++) {
    corners[i] = malloc_safe(sizeof(vector_t));
  }
  *corners[0] = (vector_t){-0.5 * size.x, -0.5 * size.y};
  *corners[1] = (vector_t){0.5 * size.x, -0.5 * size.y};
  *corners[2] = (vector_t){0.5 * size.x, 0.5 * size.y};
  *corners[3] = (vector_t){-0.5 * size.x, 0.5 * size.y};
  for (size_t i = 0; i < NUM_CORNERS; i++) {
    list_add(shape, *(corners + i));
  }
  return shape;
}

list_t *shape_ellipse(vector_t size) {
  list_t *shape = list_init(ELLIPSE_NUM_SIDES, free);
  for (size_t i = 0; i < ELLIPSE_NUM_SIDES; i++) {
    double angle = (double)i / (double)ELLIPSE_NUM_SIDES * 2.0 * PI;
    vector_t *vertex = malloc_safe(sizeof(vector_t));
    vertex->x = cos(angle) * size.x;
    vertex->y = sin(angle) * size.y;
    list_add(shape, vertex);
  }
  return shape;
}