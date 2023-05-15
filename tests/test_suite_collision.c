#include <assert.h>
#include <collision.h>
#include <math.h>
#include <stdlib.h>
#include <test_util.h>
#include <util.h>
#include <vector.h>

static const vector_t triangle1_points[] = {{0.0, 0.0}, {0.0, 1.0}, {1.0, 0.0}};

// triangle2 slightly does not overlap triangle1
static const vector_t triangle2_points[] = {{1.0, 1.0}, {0.1, 1.0}, {1.0, 0.1}};

// triangle3 overlaps with triangle1
static const vector_t triangle3_points[] = {{0.4, 0.4}, {0.4, 1.4}, {1.4, 0.4}};

static list_t *make_triangle(const vector_t *points) {
  list_t *triangle = list_init(3, free);
  for (size_t i = 0; i < 3; i++) {
    vector_t *copy = malloc_safe(sizeof(vector_t));
    *copy = points[i];
    list_add(triangle, copy);
  }
  return triangle;
}

void test_collision() {
  list_t *triangle1 = make_triangle(triangle1_points);
  list_t *triangle2 = make_triangle(triangle2_points);
  list_t *triangle3 = make_triangle(triangle3_points);

  collision_info_t collision1 = find_collision(triangle1, triangle2);
  assert(!collision1.collided);

  collision_info_t collision2 = find_collision(triangle1, triangle3);
  assert(collision2.collided);
  assert(isclose(collision2.axis.x, M_SQRT1_2));
  assert(isclose(collision2.axis.y, M_SQRT1_2));

  list_free(triangle1);
  list_free(triangle2);
  list_free(triangle3);
}

int main(int argc, char **argv) {
  // Run all tests? True if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_collision)

  puts("collision_test PASS");
}