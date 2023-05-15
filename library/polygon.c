#include <polygon.h>
#include <stdio.h>

double polygon_area(list_t *polygon) {
  double sum = 0.0;
  const size_t num_vertices = list_size(polygon);
  for (size_t i = 0; i < num_vertices; i++) {
    // see https://en.wikipedia.org/wiki/Shoelace_formula#Shoelace_formula
    sum += vec_cross(*(vector_t *)list_get(polygon, i),
                     *(vector_t *)list_get(polygon, (i + 1) % num_vertices));
  }
  return sum / 2.0;
}

vector_t polygon_centroid(list_t *polygon) {
  // formula from https://en.wikipedia.org/wiki/Centroid#Of_a_polygon

  vector_t sum_vec = VEC_ZERO;

  const size_t num_vertices = list_size(polygon);
  for (size_t i = 0; i < num_vertices; i++) {
    vector_t vertex1 = *(vector_t *)list_get(polygon, i);
    vector_t vertex2 = *(vector_t *)list_get(polygon, (i + 1) % num_vertices);

    // calculate the part inside the sum
    vector_t vector_term = vec_add(vertex1, vertex2);
    double scalar_term = vec_cross(vertex1, vertex2);
    sum_vec = vec_add(sum_vec, vec_multiply(scalar_term, vector_term));
  }

  double coeff = 1.0 / (6.0 * polygon_area(polygon));
  return vec_multiply(coeff, sum_vec);
}

void polygon_translate(list_t *polygon, vector_t translation) {
  const size_t num_vertices = list_size(polygon);
  for (size_t i = 0; i < num_vertices; i++) {
    vector_t *vertex = list_get(polygon, i);
    *vertex = vec_add(*vertex, translation);
  }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
  // in order to rotate around a point,
  // we translate the polygon such that the point is the origin
  // then, we rotate about the origin, and translate back
  polygon_translate(polygon, vec_negate(point));
  const size_t num_vertices = list_size(polygon);
  for (size_t i = 0; i < num_vertices; i++) {
    vector_t *vertex = list_get(polygon, i);
    *vertex = vec_rotate(*vertex, angle);
  }
  polygon_translate(polygon, point);
}