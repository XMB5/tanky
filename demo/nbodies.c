#include <body.h>
#include <collision.h>
#include <forces.h>
#include <scene.h>
#include <sdl_wrapper.h>
#include <shape.h>
#include <state.h>
#include <util.h>
#include <vector.h>

static const unsigned int RANDOM_SEED = 69; // srand takes unsigned int
static const vector_t SCREEN_SIZE = {1000.0, 500.0};

static const size_t NUM_STARS = 100;
static const size_t NUM_STAR_POINTS = 4;
static const double STAR_OUTER_RADIUS_MAX = 40.0;
static const double STAR_OUTER_RADIUS_MIN = 10.0;
static const double STAR_INNER_RADIUS_FRACTION = 0.4;

static const double GRAVITY = 20.0;
static const double STAR_INITIAL_VELOCITY = 50.0;

struct state {
  scene_t *scene;
};

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();

  for (size_t i = 0; i < NUM_STARS; i++) {
    double outer_radius =
        rand_range(STAR_OUTER_RADIUS_MIN, STAR_OUTER_RADIUS_MAX);
    list_t *shape =
        shape_star_create(NUM_STAR_POINTS, outer_radius,
                          STAR_INNER_RADIUS_FRACTION * outer_radius);
    double mass = outer_radius * outer_radius;
    body_t *star = body_init(shape, mass, color_random());

    vector_t pos = {
        rand_range(SCREEN_SIZE.x * 1.0 / 4.0, SCREEN_SIZE.x * 3.0 / 4.0),
        rand_range(SCREEN_SIZE.y * 1.0 / 4.0, SCREEN_SIZE.y * 3.0 / 4.0)};
    body_set_centroid(star, pos);
    scene_add_body(state->scene, star);

    // add angular momentum
    vector_t r_vec = vec_subtract(pos, vec_multiply(0.5, SCREEN_SIZE));
    body_add_impulse(star, vec_multiply(STAR_INITIAL_VELOCITY * mass,
                                        vec_norm(vec_perpendicular(r_vec))));

    for (size_t j = 0; j < i; j++) {
      create_newtonian_gravity(state->scene, GRAVITY, star,
                               scene_get_body(state->scene, j));
    }
  }

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  scene_tick(state->scene, dt);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}