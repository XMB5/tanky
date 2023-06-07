#include <forces.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

static const double GRAVITY_MIN_DISTANCE = 5.0;
static const double MAX_BULLET_BOUNCES = 3.0;

typedef struct {
  body_t *body1;
  body_t *body2;
  double constant_val;
} bodies_aux_t;

typedef struct {
  body_t *body;
  double constant_val;
} body_aux_t;

typedef struct {
  body_t *body1;
  body_t *body2;
  bool just_collided;
  collision_handler_t handler;
  void *handler_aux;
  free_func_t handler_aux_freer;
} collision_aux_t;

typedef struct {
  size_t *health;
  bool *was_shot;
} bullet_aux_t;

static void collision_aux_free(collision_aux_t *aux) {
  if (aux->handler_aux_freer && aux->handler_aux) {
    aux->handler_aux_freer(aux->handler_aux);
  }
  free(aux);
}

static void newtonian_gravity_forcer(bodies_aux_t *aux) {
  double G = aux->constant_val;

  vector_t r_vec = vec_subtract(body_get_centroid(aux->body2),
                                body_get_centroid(aux->body1));
  double dist = fmax(vec_magnitude(r_vec), GRAVITY_MIN_DISTANCE);

  double force_magnitude =
      body_get_mass(aux->body1) * body_get_mass(aux->body2) * G / (dist * dist);
  vector_t force = vec_multiply(force_magnitude, vec_norm(r_vec));

  body_add_force(aux->body1, force);
  body_add_force(aux->body2, vec_negate(force));
}

/**
 * Adds a force creator to a scene that applies gravity between two bodies.
 * The force creator will be called each tick
 * to compute the Newtonian gravitational force between the bodies.
 * See
 * https://en.wikipedia.org/wiki/Newton%27s_law_of_universal_gravitation#Vector_form.
 * The force should not be applied when the bodies are very close,
 * because its magnitude blows up as the distance between the bodies goes to 0.
 *
 * @param scene the scene containing the bodies
 * @param G the gravitational proportionality constant
 * @param body1 the first body
 * @param body2 the second body
 */
void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {
  bodies_aux_t *aux = malloc_safe(sizeof(bodies_aux_t));
  aux->body1 = body1;
  aux->body2 = body2;
  aux->constant_val = G;
  list_t *bodies = list_init(
      2, NULL); // do not add freer, because bodies will be free'd by the scene
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(
      scene, (force_creator_t)newtonian_gravity_forcer, aux, bodies, free);
}

static void spring_forcer(bodies_aux_t *aux) {
  double k = aux->constant_val;

  vector_t r_vec = vec_subtract(body_get_centroid(aux->body2),
                                body_get_centroid(aux->body1));
  vector_t force = vec_multiply(k, r_vec);

  body_add_force(aux->body1, force);
  body_add_force(aux->body2, vec_negate(force));
}

/**
 * Adds a force creator to a scene that acts like a spring between two bodies.
 * The force creator will be called each tick
 * to compute the Hooke's-Law spring force between the bodies.
 * See https://en.wikipedia.org/wiki/Hooke%27s_law.
 *
 * @param scene the scene containing the bodies
 * @param k the Hooke's constant for the spring
 * @param body1 the first body
 * @param body2 the second body
 */
void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
  bodies_aux_t *aux = malloc_safe(sizeof(bodies_aux_t));
  aux->body1 = body1;
  aux->body2 = body2;
  aux->constant_val = k;
  list_t *bodies = list_init(
      2, NULL); // do not add freer, because bodies will be free'd by the scene
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)spring_forcer, aux,
                                 bodies, free);
}

static void drag_forcer(body_aux_t *aux) {
  double gamma = aux->constant_val;
  body_add_force(aux->body, vec_multiply(-gamma, body_get_velocity(aux->body)));
}

/**
 * Adds a force creator to a scene that applies a drag force on a body.
 * The force creator will be called each tick
 * to compute the drag force on the body proportional to its velocity.
 * The force points opposite the body's velocity.
 *
 * @param scene the scene containing the bodies
 * @param gamma the proportionality constant between force and velocity
 *   (higher gamma means more drag)
 * @param body the body to slow down
 */
void create_drag(scene_t *scene, double gamma, body_t *body) {
  body_aux_t *aux = malloc_safe(sizeof(body_aux_t));
  aux->body = body;
  aux->constant_val = gamma;
  list_t *bodies = list_init(
      1, NULL); // do not add freer, because bodies will be free'd by the scene
  list_add(bodies, body);
  scene_add_bodies_force_creator(scene, (force_creator_t)drag_forcer, aux,
                                 bodies, free);
}

static void collision_forcer(collision_aux_t *aux) {
  collision_info_t info = find_collision(body_get_shape_unsafe(aux->body1),
                                         body_get_shape_unsafe(aux->body2));
  if (info.collided) {
    if (!aux->just_collided) {
      aux->handler(aux->body1, aux->body2, info.axis, aux->handler_aux);
      aux->just_collided = true;
    }
  } else {
    aux->just_collided = false;
  }
  // if (aux->just_collided) {
  //   aux->just_collided = false;
  // } else if (info.collided) {
  //   aux->just_collided = true;
  //   aux->handler(aux->body1, aux->body2, info.axis, aux->handler_aux);
  // }
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  collision_aux_t *collision_aux = malloc_safe(sizeof(collision_aux_t));
  collision_aux->body1 = body1;
  collision_aux->body2 = body2;
  collision_aux->just_collided = false;
  collision_aux->handler = handler;
  collision_aux->handler_aux = aux;
  collision_aux->handler_aux_freer = freer;
  list_t *bodies = list_init(
      2, NULL); // do not add freer, because bodies will be free'd by the scene
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)collision_forcer,
                                 collision_aux, bodies,
                                 (free_func_t)collision_aux_free);
}

static void destructive_collision_handler(body_t *body1, body_t *body2,
                                          vector_t axis, void *aux) {
  body_remove(body1);
  body_remove(body2);
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
  create_collision(scene, body1, body2, destructive_collision_handler, NULL,
                   NULL);
}

static void bullet_tank_collision_handler(body_t *tank, body_t *bullet,
                                          vector_t axis, bullet_aux_t *aux) {
  *aux->health = *aux->health - 1;
  *aux->was_shot = true;
  body_remove(bullet);
}

void create_bullet_tank_collision(scene_t *scene, body_t *tank, body_t *bullet,
                                  size_t *health, bool *was_shot) {
  bullet_aux_t *aux = malloc(sizeof(bullet_aux_t));
  aux->health = health;
  aux->was_shot = was_shot;
  create_collision(scene, tank, bullet,
                   (collision_handler_t)bullet_tank_collision_handler, aux,
                   free);
}

static void bullet_obstacle_collision_handler(body_t *tank, body_t *bullet,
                                              vector_t axis,
                                              bullet_aux_t *aux) {
  body_remove(bullet);
}

void create_bullet_obstacle_collision(scene_t *scene, body_t *tank,
                                      body_t *bullet) {
  bullet_aux_t *aux = malloc(sizeof(bullet_aux_t));
  create_collision(scene, tank, bullet,
                   (collision_handler_t)bullet_obstacle_collision_handler, aux,
                   free);
}

void physics_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                               const double *elasticity) {
  double ua = vec_dot(body_get_velocity(body1), axis);
  double ub = vec_dot(body_get_velocity(body2), axis);

  double massA = body_get_mass(body1);
  double massB = body_get_mass(body2);
  double reduced_mass;
  if (massA == INFINITY) {
    reduced_mass = massB;
  } else if (massB == INFINITY) {
    reduced_mass = massA;
  } else {
    reduced_mass = (massA * massB) / (massA + massB);
  }

  vector_t impulse =
      vec_multiply(reduced_mass * (1 + *elasticity) * (ub - ua), axis);
  body_add_impulse(body1, impulse);
  body_add_impulse(body2, vec_negate(impulse));
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
  double *elasticity_aux = malloc_safe(sizeof(double));
  *elasticity_aux = elasticity;
  create_collision(scene, body1, body2,
                   (collision_handler_t)physics_collision_handler,
                   elasticity_aux, free);
}


void bullet_collision_handler(body_t *bullet, body_t *wall, vector_t axis,
                               const double *elasticity) {
  *(size_t*)bullet->info += 1;
  if(*(size_t*)bullet->info > MAX_BULLET_BOUNCES){
    body_remove(bullet);
  }
  physics_collision_handler(bullet, wall, axis, elasticity);  
}

void create_bullet_wall_collision(scene_t *scene, double elasticity, body_t *bullet,
                              body_t *wall) {
  double *elasticity_aux = malloc_safe(sizeof(double));
  *elasticity_aux = elasticity;
  create_collision(scene, bullet, wall,
                   (collision_handler_t)bullet_collision_handler,
                   elasticity_aux, free);
}

