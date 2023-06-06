#ifndef __SOUND_H__
#define __SOUND_H__

#include <SDL2/SDL_mixer.h>

typedef struct {
    const char *name;
    Mix_Chunk *chunk;
} sound_t;

void sound_init();
sound_t *sound_load(const char *name);
void sound_play(const char *name);
void sound_deinit();

#endif