#include <assert.h>
#include <color.h>
#include <stdlib.h>
#include <test_util.h>

void test_random_color() {
  srand(0);
  rgb_color_t color = color_random();
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);
}

int main(int argc, char **argv) {
  // Run all tests? True if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_random_color)

  puts("color_test PASS");
}