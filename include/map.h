#ifndef __MAP_H__
#define __MAP_H__

#include <scene.h>
#include <vector.h>

extern const char *BODY_TYPE_WALL;

void map_add_walls(scene_t *scene, vector_t screen_size);

#endif