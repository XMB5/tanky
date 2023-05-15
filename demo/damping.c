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

static const size_t NUM_BALLS = 200;
static const double BALL_MASS = 1.0;
static const vector_t BALL_INITIAL_IMPULSE = {0.0, 100.0};

static const double SPRING_CONST_MAX = 5.0;
static const double DRAG_MAX = 0.1;

struct state {
  scene_t *scene;
};

state_t *emscripten_init() {
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  srand(RANDOM_SEED);

  state_t *state = malloc_safe(sizeof(state_t));
  state->scene = scene_init();

  double ball_radius = SCREEN_SIZE.x / NUM_BALLS / 2.0;
  for (size_t i = 0; i < NUM_BALLS; i++) {
    list_t *shape = shape_circle_create(ball_radius);
    body_t *ball = body_init(shape, BALL_MASS, color_random());

    list_t *ghost_shape = shape_circle_create(0.0);
    body_t *ghost_spring_base =
        body_init(ghost_shape, INFINITY, COLOR_WHITE); // not added to scene

    vector_t pos = {ball_radius * 2.0 * (i + 0.5), SCREEN_SIZE.y / 2.0};
    body_set_centroid(ball, pos);
    body_set_centroid(ghost_spring_base, pos);
    scene_add_body(state->scene, ball);

    body_add_impulse(ball, BALL_INITIAL_IMPULSE);

    double spring_constant = SPRING_CONST_MAX * (1.0 - (double)i / NUM_BALLS);
    create_spring(state->scene, spring_constant, ball, ghost_spring_base);

    double drag_constant = DRAG_MAX * ((double)i / NUM_BALLS);
    create_drag(state->scene, drag_constant, ball);
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