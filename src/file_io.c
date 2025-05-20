#include "file_io.h"
#include "types.h"
#include "error_handling.h"
#include <SDL2/SDL_image.h>

/*
    Load image from file and create texture using it
 */
void load_texture(SDL_Texture **txt, const char *filepath) {
    SDL_Surface *srf = IMG_Load(filepath);
    if (srf == NULL) {
        set_error("Error loading image '%s': %s", filepath, IMG_GetError());
        return;
    }
    *txt = SDL_CreateTextureFromSurface(g_gfx.renderer, srf);
    if (*txt == NULL) {
        set_error("Error creating texture from '%s': %s", filepath, SDL_GetError());
    }
    SDL_FreeSurface(srf);
}
