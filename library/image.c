#include <image.h>
#include <stdio.h>
#include <list.h>
#include <util.h>
#include <sdl_wrapper.h>
#include <SDL2/SDL_image.h>

static list_t *image_cache;

void free_image(image_t *loaded_image) {
    SDL_DestroyTexture(loaded_image->texture);
    free(loaded_image);
}

void image_init() {
    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        printf("SDL IMG_Init failed: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    const size_t images_initial_capacity = 20;
    image_cache = list_init(images_initial_capacity, (free_func_t) free_image);
}

void image_deinit() {
    list_free(image_cache);
    IMG_Quit();
}

image_t *image_load(const char *name) {
    // check if already loaded
    size_t num_images = list_size(image_cache);
    for (size_t i = 0; i < num_images; i++) {
        image_t *image = list_get(image_cache, i);
        if (strcmp(image->name, name) == 0) {
            return image;
        }
    }

    // load from file
    const size_t MAX_PATH_LEN = 256;
    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "assets/image/%s.png", name);
    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        printf("SDL IMG_Load failed: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(sdl_renderer(), surface);

    image_t *image = malloc_safe(sizeof(image_t));
    image->texture = texture;
    image->name = name;
    image->w = surface->w;
    image->h = surface->h;
    list_add(image_cache, image);

    SDL_FreeSurface(surface);

    return image;
}