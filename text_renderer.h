#ifndef __TEXT_RENDERER_H__
#define __TEXT_RENDERER_H__

typedef enum e_TextAlignment {
    Left,
    Center,
    Right
} TextAlignment;

void init_text_renderer(SDL_Renderer* renderer, SDL_Texture* texture);
int render_text(int x, int y, TextAlignment align, char *text);

#endif
