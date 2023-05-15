#include <collision.h>
#include <math.h>
#include <vector.h>

typedef struct {
  double min;
  double max;
} projection_range_t;

static projection_range_t shape_project(list_t *shape, vector_t axis) {
  size_t shape1_size = list_size(shape);
  projection_range_t range = {INFINITY, -INFINITY};

  for (size_t i = 0; i < shape1_size; i++) {
    double projection = vec_dot(*(vector_t *)list_get(shape, i), axis);
    range.max = fmax(range.max, projection);
    range.min = fmin(range.min, projection);
  }

  return range;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
  size_t shape1_size = list_size(shape1);
  size_t shape2_size = list_size(shape2);

  double collision_axis_overlap = INFINITY;
  vector_t collision_axis;

  for (size_t i = 0; i < shape1_size + shape2_size; i++) {
    vector_t *point1;
    vector_t *point2;
    if (i < shape1_size) {
      point1 = list_get(shape1, i);
      point2 = list_get(shape1, (i + 1) % shape1_size);
    } else {
      point1 = list_get(shape2, i - shape1_size);
      point2 = list_get(shape2, (i - shape1_size + 1) % shape2_size);
    }

    vector_t edge = vec_subtract(*point2, *point1);
    vector_t axis = vec_norm(vec_perpendicular(edge));
    if (i >= shape1_size) {
      // so that normal points from shape 1 to shape 2
      axis = vec_negate(axis);
    }

    projection_range_t range1 = shape_project(shape1, axis);
    projection_range_t range2 = shape_project(shape2, axis);

    double overlap =
        fmin(range1.max, range2.max) - fmax(range1.min, range2.min);
    if (overlap < 0) {
      collision_info_t no_collision = {false, VEC_ZERO};
      return no_collision;
    } else if (overlap < collision_axis_overlap) {
      collision_axis_overlap = overlap;
      collision_axis = axis;
    }
  }

  collision_info_t collision = {true, collision_axis};
  return collision;
}