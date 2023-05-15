#include <math.h>
#include <vector.h>

const vector_t VEC_ZERO = {0.0, 0.0};

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t result = {v1.x + v2.x, v1.y + v2.y};
  return result;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  return vec_add(v1, vec_negate(v2));
}

vector_t vec_negate(vector_t v) {
  vector_t result = {-v.x, -v.y};
  return result;
}

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t result = {scalar * v.x, scalar * v.y};
  return result;
}

vector_t vec_divide(double scalar, vector_t v) {
  vector_t result = {v.x / scalar, v.y / scalar};
  return result;
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v1.y * v2.x; }

vector_t vec_rotate(vector_t v, double angle) {
  double sin_theta = sin(angle);
  double cos_theta = cos(angle);
  vector_t result = {v.x * cos_theta - v.y * sin_theta,
                     v.x * sin_theta + v.y * cos_theta};
  return result;
}

vector_t vec_perpendicular(vector_t vec) {
  vector_t perp = {-vec.y, vec.x};
  return perp;
}

double vec_magnitude(vector_t vec) { return sqrt(vec_dot(vec, vec)); }

vector_t vec_norm(vector_t vec) {
  return vec_multiply(1.0 / vec_magnitude(vec), vec);
}