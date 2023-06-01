#include <font.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <sdl_wrapper.h>

static TTF_Font *font;

void font_init() {
    TTF_Init();
    const int point_size = 20;
    font = TTF_OpenFont("assets/fonts/Roboto-Black.ttf", point_size);
    if (!font) {
        printf("TTF_OpenFont failed\n");
        exit(EXIT_FAILURE);
    }
}

void font_render(const char *text, vector_t top_left, rgb_color_t color) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color_to_sdl(color));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(sdl_renderer(), surface);
    
    SDL_Rect dstrect;
    dstrect.h = surface->h;
    dstrect.w = surface->w;
    dstrect.x = top_left.x;
    dstrect.y = top_left.y;

    SDL_RenderCopy(sdl_renderer(), texture, NULL, &dstrect);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void font_deinit() {
    TTF_Quit();
}