#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "text_renderer.h"
#include "font_layout.h"

static SDL_Renderer* text_renderer = NULL;
static SDL_Texture* font_texture = NULL;
static int font_h = 0; /* font texture height */

void init_text_renderer(SDL_Renderer* renderer, SDL_Texture* texture) {
    text_renderer = renderer;
    font_texture = texture;
    SDL_QueryTexture(font_texture, NULL, NULL, NULL, &font_h);
}

int render_text(int x, int y, TextAlignment align, char *text) {
    int i; /* index of letter font layout table */
    char *text_p = NULL;
    int text_w = 0; /* text width */
    SDL_Rect SrcR, DstR;

    SrcR.y = 0;
    DstR.y = y;
    SrcR.h = DstR.h = font_h;

    /* calculate text width */
    text_p = text;
    while(*text_p) {
        if (*text_p != ' ') {
            i = *text_p - '!';
            text_w += font_layout[i+1] - font_layout[i];
        }
        else {
            text_w += font_h/2; /* substitute for space */
        }
        text_p++;
    }

    /* start position of text */
    if (align == Right) {
        DstR.x = x - text_w;
    }
    else if (align == Center) {
        DstR.x = x - text_w/2;
    }
    else {
        DstR.x = x;
    }

    /* Render text */
    text_p = text;
    while(*text_p) {
        if (*text_p != ' ') {
            i = *text_p - '!';
            SrcR.x = font_layout[i];
            SrcR.w = DstR.w = font_layout[i+1] - font_layout[i];
            SDL_RenderCopy(text_renderer, font_texture, &SrcR, &DstR);
            DstR.x += DstR.w;
        }
        else {
            DstR.x += font_h/3; /* substitute for space */
        }
        text_p++;
    }

    return text_w;
}
