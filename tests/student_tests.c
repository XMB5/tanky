#include <assert.h>
#include <body.h>
#include <forces.h>
#include <math.h>
#include <scene.h>
#include <stdlib.h>
#include <test_util.h>
#include <vector.h>

list_t *make_shape() {
  list_t *shape = list_init(4, free);
  vector_t *v = malloc(sizeof(*v));
  *v = (vector_t){-1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, +1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){-1, +1};
  list_add(shape, v);
  return shape;
}

// Tests that drag decelerates a mass over time
void test_drag_deceleration() {
  const double M = 10;
  const double GAMMA = 2;
  const double DT = 1e-3;
  const int STEPS = 10000;
  const double INITIAL_VEL = 5.0;
  scene_t *scene = scene_init();
  body_t *body = body_init(make_shape(), M, (rgb_color_t){0, 0, 0});
  body_set_velocity(body, (vector_t){INITIAL_VEL, 0});
  scene_add_body(scene, body);
  create_drag(scene, GAMMA, body);

  for (int i = 0; i < STEPS; i++) {
    scene_tick(scene, DT);
  }
  assert(body_get_velocity(body).x < INITIAL_VEL / 2.0);

  scene_free(scene);
}

// Tests that a body without initial displacement/velocity will remain at rest
// when anchored to a heavy body on a spring
void test_spring_equilibrium() {
  const double M = 10;
  const double K = 2;
  const double DT = 1e-5;
  const int STEPS = 10000;
  scene_t *scene = scene_init();
  body_t *mass = body_init(make_shape(), M, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass, VEC_ZERO);
  scene_add_body(scene, mass);
  body_t *anchor = body_init(make_shape(), INFINITY, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, anchor);
  create_spring(scene, K, mass, anchor);
  for (int i = 0; i < STEPS; i++) {
    assert(vec_isclose(body_get_centroid(mass), VEC_ZERO));
    assert(vec_isclose(body_get_centroid(anchor), VEC_ZERO));
    scene_tick(scene, DT);
  }

  scene_free(scene);
}

// Tests that an object will orbit around a heavy object
void test_newtonian_gravity_orbit() {
  const double M1 = 1e8, M2 = 1e-6;
  const double G = 1e3;
  const double DT = 1e-6;
  const double ORBIT_RADIUS = 50.0;
  const int STEPS = 10000;
  scene_t *scene = scene_init();
  body_t *body1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, body1);
  body_t *body2 = body_init(make_shape(), M2, (rgb_color_t){0, 0, 0});
  body_set_centroid(body2, (vector_t){ORBIT_RADIUS, 0});
  // M1 >> M2, thus we use approximate velocity formula
  // the orbit won't be perfectly circular
  body_set_velocity(body2, (vector_t){0, sqrt(G * M1 / (ORBIT_RADIUS))});
  scene_add_body(scene, body2);
  create_newtonian_gravity(scene, G, body1, body2);

  for (int i = 0; i < STEPS; i++) {
    scene_tick(scene, DT);
  }

  assert(within(1.0,
                vec_magnitude(vec_subtract(body_get_centroid(body1),
                                           body_get_centroid(body2))),
                ORBIT_RADIUS));

  scene_free(scene);
}

int main(int argc, char *argv[]) {
  // Tests that drag decelerates a mass over time
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_drag_deceleration)
  DO_TEST(test_spring_equilibrium)
  DO_TEST(test_newtonian_gravity_orbit)

  puts("student_tests PASS");
}