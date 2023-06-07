#ifndef __MAP_H__
#define __MAP_H__

#include <scene.h>
#include <vector.h>

extern const char *BODY_TYPE_WALL;
extern const char *BODY_TYPE_OBSTACLE;

void map_add_walls(scene_t *scene, vector_t screen_size);

void map_init_obstacles(scene_t *scene, vector_t screen_size, size_t num_obstacles);

void map_reset_obstacles(scene_t *scene, vector_t screen_size, size_t num_obstacles);

#endif