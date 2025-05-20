#ifndef __TEXT_RENDERER_H__
#define __TEXT_RENDERER_H__

#include <SDL2/SDL.h>

typedef enum e_TextHorizontalAlignment {
    ALIGN_LEFT,
    ALIGN_CENTER_HORIZONTAL,
    ALIGN_RIGHT
} TextHorizontalAlignment;

typedef enum e_TextVerticalAlignment {
    ALIGN_TOP,
    ALIGN_CENTER_VERTICAL,
    ALIGN_BOTTOM
} TextVerticalAlignment;

void init_text_renderer(SDL_Renderer* renderer, SDL_Texture* texture);
void set_text_color(uint8_t r, uint8_t g, uint8_t b);
int render_text(SDL_Renderer* renderer, SDL_Texture* font, const char* text, int x, int y, TextHorizontalAlignment horizontal_align, TextVerticalAlignment vertical_align);
int get_text_width(const char* text);
int get_text_height(const char* text);

#define SET_WHITE_TEXT set_text_color(224, 224, 224)
#define SET_GREY_TEXT set_text_color(128, 128, 128)
#define SET_YELLOW_TEXT set_text_color(255, 192, 32)

#endif
