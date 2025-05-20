#ifndef __TEXT_RENDERER_H__
#define __TEXT_RENDERER_H__

#include <SDL2/SDL.h>
#include "types.h"
void init_text_renderer(SDL_Renderer* renderer, SDL_Texture* texture);
int render_text(SDL_Renderer* renderer, SDL_Texture* font, const char* text, int x, int y, TextHorizontalAlignment horizontal_align, TextVerticalAlignment vertical_align, TextColor color);
int get_text_width(const char* text);
int get_text_height(const char* text);

#endif
