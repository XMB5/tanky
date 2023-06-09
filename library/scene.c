#include <list.h>
#include <scene.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

static const size_t INITIAL_LIST_CAPACITY = 100; // approx number of bodies

typedef struct {
  force_creator_t forcer;
  void *aux;
  free_func_t freer;
  list_t *bodies;
} force_info_t;

static void force_info_free(force_info_t *force_info) {
  if (force_info->bodies) {
    list_free(force_info->bodies);
  }
  if (force_info->freer && force_info->aux) {
    force_info->freer(force_info->aux);
  }
  free(force_info);
}

void scene_text_to_draw_free(text_to_draw_t *text_to_draw) {
  free((void*) text_to_draw->text);
  free(text_to_draw);
}


struct scene {
  list_t *bodies;
  list_t *force_creators;
  list_t *texts_to_draw;
  list_t *images_to_draw;
};

scene_t *scene_init(void) {
  scene_t *scene = malloc_safe(sizeof(scene_t));
  scene->bodies = list_init(INITIAL_LIST_CAPACITY, (free_func_t)body_free);
  scene->force_creators =
      list_init(INITIAL_LIST_CAPACITY, (free_func_t)force_info_free);
  scene->texts_to_draw = list_init(INITIAL_LIST_CAPACITY, (free_func_t)scene_text_to_draw_free);
  scene->images_to_draw = list_init(INITIAL_LIST_CAPACITY, (free_func_t)free);
  return scene;
}

void scene_free(scene_t *scene) {
  list_free(scene->bodies);
  list_free(scene->force_creators);
  list_free(scene->texts_to_draw);
  list_free(scene->images_to_draw);
  free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
  return list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
  body_remove(list_get(scene->bodies, index));
}

static void scene_remove_body_real(scene_t *scene, size_t index) {
  body_t *removed_body = list_remove(scene->bodies, index);

  // remove force creator if one of its bodies is removed
  size_t num_forcers = list_size(scene->force_creators);
  for (size_t forcer_num = 0; forcer_num < num_forcers; forcer_num++) {
    force_info_t *force_info = list_get(scene->force_creators, forcer_num);

    bool remove_forcer = false;
    if (force_info->bodies) {
      size_t num_forcer_bodies = list_size(force_info->bodies);
      for (size_t body_num = 0; body_num < num_forcer_bodies; body_num++) {
        if (list_get(force_info->bodies, body_num) == removed_body) {
          remove_forcer = true;
          break;
        }
      }
    }

    if (remove_forcer) {
      force_info_free(list_remove(scene->force_creators, forcer_num));
      forcer_num--;
      num_forcers--;
    }
  }

  body_free(removed_body);
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
  scene_add_bodies_force_creator(scene, forcer, aux, NULL, freer);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  force_info_t *force_info = malloc_safe(sizeof(force_info_t));
  force_info->forcer = forcer;
  force_info->aux = aux;
  force_info->freer = freer;
  force_info->bodies = bodies;
  list_add(scene->force_creators, force_info);
}

void scene_draw_text(scene_t *scene, const char *text, vector_t top_left, rgb_color_t color) {
  text_to_draw_t *to_draw = malloc_safe(sizeof(text_to_draw_t));
  to_draw->text = strdup_safe(text);
  to_draw->top_left = top_left;
  to_draw->color = color;
  list_add(scene->texts_to_draw, to_draw);
}

list_t *scene_get_texts_to_draw(scene_t *scene) {
  return scene->texts_to_draw;
}

void scene_draw_image(scene_t *scene, image_t *image, vector_t pos, double scale, double rotation) {
  image_to_draw_t *to_draw = malloc_safe(sizeof(image_to_draw_t));
  to_draw->image = image;
  to_draw->pos = pos;
  to_draw->scale = scale;
  to_draw->rotation = rotation;
  list_add(scene->images_to_draw, to_draw);
}

list_t *scene_get_images_to_draw(scene_t *scene) {
  return scene->images_to_draw;
}

void scene_tick(scene_t *scene, double dt) {
  size_t num_forcers = list_size(scene->force_creators);
  for (size_t i = 0; i < num_forcers; i++) {
    force_info_t *force_info = list_get(scene->force_creators, i);
    force_info->forcer(force_info->aux);
  }

  size_t num_bodies = scene_bodies(scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_is_removed(body)) {
      scene_remove_body_real(scene, i);
      i--;
      num_bodies--;
    } else {
      body_tick(body, dt);
    }
  }
}