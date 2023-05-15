#include <assert.h>
#include <shape.h>
#include <stdlib.h>
#include <test_util.h>

void test_shape_star_create() {
  size_t num_points = 10;
  double inner_radius = 50.0;
  double outer_radius = 20.0;
  list_t *shape = shape_star_create(num_points, inner_radius, outer_radius);
  assert(list_size(shape) == num_points * 2);
  list_free(shape);
}

int main(int argc, char **argv) {
  // Run all tests? True if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_shape_star_create)

  puts("shape_test PASS");
}