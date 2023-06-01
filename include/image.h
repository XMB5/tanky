#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <SDL2/SDL.h>

typedef struct {
    const char *name;
    SDL_Texture *texture;
    int w;
    int h;
} image_t;

void image_init();
void image_deinit();
image_t *image_load(const char *name);

#endif