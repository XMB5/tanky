#include <assert.h>
#include <stdlib.h>
#include <test_util.h>
#include <util.h>

void test_malloc_safe() {
  void *data = malloc_safe(100);
  assert(data != NULL);
  free(data);
}

int main(int argc, char **argv) {
  // Run all tests? True if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_malloc_safe)

  puts("util_test PASS");
}