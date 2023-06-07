#include <body.h>
#include <collision.h>
#include <list.h>
#include <polygon.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util.h>
#include <vector.h>
#include <image.h>

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  return body_init_with_info(shape, mass, color, NULL);
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color, const char *type) {
  body_t *body = malloc_safe(sizeof(body_t));
  body->shape = shape;
  body->mass = mass;
  body->color = color;
  body->vel = VEC_ZERO;
  body->pos = polygon_centroid(shape);
  body->angular_vel = 0.0;
  body->angle = 0.0;
  body->net_force = VEC_ZERO;
  body->net_impulse = VEC_ZERO;
  body->info = NULL;
  body->freer = NULL;
  body->removed = false;
  body->image = NULL;
  body->type = type;
  return body;
}

void body_free(body_t *body) {
  list_free(body->shape);
  if (body->freer != NULL && body->info != NULL) {
    body->freer(body->info);
  }
  free(body);
}

list_t *body_get_shape(body_t *body) {
  size_t size = list_size(body->shape);
  list_t *new = list_init(size, free);
  for (size_t i = 0; i < size; i++) {
    vector_t *vec = list_get(body->shape, i);
    vector_t *vec_copy = malloc_safe(sizeof(vector_t));
    *vec_copy = *vec;
    list_add(new, vec_copy);
  }
  return new;
}

list_t *body_get_shape_unsafe(body_t *body) { return body->shape; }

double body_get_mass(body_t *body) { return body->mass; }

collision_info_t body_collide(body_t *body1, body_t *body2) {
  return find_collision(body1->shape, body2->shape);
}

vector_t body_get_centroid(body_t *body) { return body->pos; }

vector_t body_get_velocity(body_t *body) { return body->vel; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

void *body_get_info(body_t *body) { return body->info; }

void body_set_centroid(body_t *body, vector_t x) {
  vector_t diff = vec_subtract(x, body->pos);
  polygon_translate(body->shape, diff);
  body->pos = x;
}

void body_set_velocity(body_t *body, vector_t v) { body->vel = v; }

void body_set_rotation(body_t *body, double angle) {
  double diff = angle - body->angle;
  polygon_rotate(body->shape, diff, body->pos);
  body->angle = angle;
}

void body_set_angular_velocity(body_t *body, double angular_velocity) {
  body->angular_vel = angular_velocity;
}

void body_tick(body_t *body, double dt) {
  vector_t acc = vec_divide(body->mass, body->net_force);
  vector_t new_vel = vec_add(vec_add(body->vel, vec_multiply(dt, acc)),
                             vec_divide(body->mass, body->net_impulse));
  vector_t dx = vec_multiply(0.5 * dt, vec_add(new_vel, body->vel));

  polygon_translate(body->shape, dx);
  body->pos = vec_add(body->pos, dx);
  body->vel = new_vel;
  body->net_force = VEC_ZERO;
  body->net_impulse = VEC_ZERO;

  double d_theta = dt * body->angular_vel;
  body->angle += d_theta;
  polygon_rotate(body->shape, d_theta, body->pos);
}

void body_tick_with_bounds(body_t *body, double dt, vector_t bounds,
                           double bounciness) {
  body_tick(body, dt);

  const size_t num_vertices = list_size(body->shape);
  bool vertical_hit = false;
  bool horizontal_hit = false;
  for (size_t i = 0; i < num_vertices; i++) {
    vector_t *vertex = list_get(body->shape, i);
    if (vertex->x < 0 || vertex->x > bounds.x) {
      horizontal_hit = true;
    }
    if (vertex->y < 0 || vertex->y > bounds.y) {
      vertical_hit = true;
    }
  }
  if (vertical_hit) {
    body->vel.y *= -bounciness;
  }
  if (horizontal_hit) {
    body->vel.x *= -bounciness;
  }
  if (vertical_hit || horizontal_hit) {
    // undo the move so that we are no longer inside the wall
    vector_t dx = vec_multiply(dt, body->vel);
    body->pos = vec_add(body->pos, dx);
    polygon_translate(body->shape, dx);
    body->color = color_random();
  }
}

void body_add_force(body_t *body, vector_t force) {
  body->net_force = vec_add(body->net_force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
  body->net_impulse = vec_add(body->net_impulse, impulse);
}

void body_remove(body_t *body) { body->removed = true; }

bool body_is_removed(body_t *body) { return body->removed; }

void body_set_image(body_t *body, const char *name, double scale) {
  body->image = image_load(name);
  body->image_scale = scale;
}

void body_set_image_rotation(body_t *body, double rotation) {
  body->image_rotation = rotation;
}

image_t *body_get_image(body_t *body) {
  return body->image;
}

double body_get_image_scale(body_t *body) {
  return body->image_scale;
}

double body_get_angle(body_t *body) {
  return body->angle;
}

double body_get_image_rotation(body_t *body) {
  return body->image_rotation;
}

void body_set_image_offset(body_t *body, vector_t offset) {
  body->image_offset = offset;
}

vector_t body_get_image_offset(body_t *body) {
  return body->image_offset;
}