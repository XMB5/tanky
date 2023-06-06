#include <sound.h>
#include <stdio.h>
#include <util.h>
#include <list.h>

static list_t *sound_cache;

void free_sound(sound_t *loaded_sound) {
    Mix_FreeChunk(loaded_sound->chunk);
    free(loaded_sound);
}

void sound_init() {
    const int frequency = 48000;
    const int channels = 2;
    const int chunkSize = 2048;
    if (Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, channels, chunkSize) < 0) {
        printf("SDL Mix_OpenAudio failed: %s\n", Mix_GetError());
        exit(EXIT_FAILURE);
    }

    const size_t sounds_initial_capacity = 20;
    sound_cache = list_init(sounds_initial_capacity, (free_func_t) free_sound);
}

sound_t *sound_load(const char *name) {
    // check if already loaded
    size_t num_sounds = list_size(sound_cache);
    for (size_t i = 0; i < num_sounds; i++) {
        sound_t *sound = list_get(sound_cache, i);
        if (strcmp(sound->name, name) == 0) {
            return sound;
        }
    }

    // load from file
    const size_t MAX_PATH_LEN = 256;
    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "assets/sounds/%s.wav", name);
    Mix_Chunk *chunk = Mix_LoadWAV(path);
    if (!chunk) {
        printf("SDL Mix_LoadWAV failed: %s\n", Mix_GetError());
        exit(EXIT_FAILURE);
    }

    sound_t *sound = malloc_safe(sizeof(sound_t));
    sound->chunk = chunk;
    sound->name = name;
    list_add(sound_cache, sound);

    return sound;
}

void sound_play(const char *name) {
    sound_t *sound = sound_load(name);
    Mix_PlayChannel(-1, sound->chunk, 0);
}

void sound_deinit() {
    list_free(sound_cache);
    Mix_Quit();
}