#ifndef __SHAPE_H__
#define __SHAPE_H__

#include <list.h>
#include <vector.h>

list_t *shape_star_create(size_t star_points, double outer_radius,
                          double inner_radius);

list_t *shape_pacman_create(double radius);
list_t *shape_arc_sweep(double radius, double empty_angle);
list_t *shape_circle_create(double radius);
list_t *shape_rectangle(vector_t size);
list_t *shape_ellipse(vector_t size);

#endif /* __SHAPE_H__ */