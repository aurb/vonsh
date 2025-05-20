#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "types.h"
#include "error_handling.h"
#include "text_renderer.h"
#include "font_layout.h"

static SDL_Renderer* text_renderer = NULL;
static SDL_Texture* font_texture = NULL;
static int font_h = 0; /* font texture height */

void init_text_renderer(SDL_Renderer* renderer, SDL_Texture* texture) {
    text_renderer = renderer;
    font_texture = texture;
    if (SDL_QueryTexture(font_texture, NULL, NULL, NULL, &font_h)) {
        set_error("Failed to query texture: %s", SDL_GetError());
    }
}

int get_text_width(const char* text) {
    int i; /* index of letter font layout table */
    const char* text_p = text;
    int text_w = 0; /* current line width */
    int max_w = 0; /* width of longest line */

    while(*text_p) {
        if (*text_p == '\n') {
            /* End of line - update max width and reset current width */
            if (text_w > max_w) {
                max_w = text_w;
            }
            text_w = 0;
        }
        else if (*text_p != ' ') {
            i = *text_p - '!';
            text_w += font_layout[i+1] - font_layout[i];
        }
        else {
            text_w += font_h/3; /* substitute for space */
        }
        text_p++;
    }

    /* Check final line width */
    if (text_w > max_w) {
        max_w = text_w;
    }

    return max_w;
}

int get_text_height(const char* text) {
    const char* text_p = text;
    int line_count = 1;  // Start at 1 for first line
    
    while(*text_p) {
        if (*text_p == '\n') {
            line_count++;
        }
        text_p++;
    }
    
    return line_count * font_h;
}

int render_text(SDL_Renderer* renderer, SDL_Texture* font, const char* text, int x, int y, TextHorizontalAlignment horizontal_align, TextVerticalAlignment vertical_align, TextColor color) {
    int i; /* index of letter font layout table */
    const char* text_p = text;
    const char* line_start = text;
    int text_h = get_text_height(text);
    SDL_Rect SrcR, DstR;
    int line_y = y;
    int line_width = 0;
    int max_width = 0;

    SrcR.y = 0;
    SrcR.h = DstR.h = font_h;

    switch (color) {
        case TEXT_WHITE:
            if (SDL_SetTextureColorMod(font_texture, 224, 224, 224)) {
                set_error("Failed to set texture color: %s", SDL_GetError());
            }
            break;
        case TEXT_GREY:
            if (SDL_SetTextureColorMod(font_texture, 128, 128, 128)) {
                set_error("Failed to set texture color: %s", SDL_GetError());
            }
            break;
        case TEXT_YELLOW:
            if (SDL_SetTextureColorMod(font_texture, 255, 192, 32)) {
                set_error("Failed to set texture color: %s", SDL_GetError());
            }
            break;
    }

    /* start y position of text block */
    if (vertical_align == ALIGN_TOP) {
        line_y = y;
    }
    else if (vertical_align == ALIGN_CENTER_VERTICAL) {
        line_y = y - text_h/2;
    }
    else if (vertical_align == ALIGN_BOTTOM) {
        line_y = y - text_h;
    }

    /* Render text characters line by line */
    while(*text_p) {
        if (*text_p == '\n' || *text_p == '\0') {
            /* Calculate start x position for this line */
            if (horizontal_align == ALIGN_RIGHT) {
                DstR.x = x - line_width;
            }
            else if (horizontal_align == ALIGN_CENTER_HORIZONTAL) {
                DstR.x = x - line_width/2;
            }
            else { /* ALIGN_LEFT */
                DstR.x = x;
            }
            DstR.y = line_y;

            /* Render the line */
            while(line_start < text_p) {
                if (*line_start != ' ') {
                    i = *line_start - '!';
                    SrcR.x = font_layout[i];
                    SrcR.w = DstR.w = font_layout[i+1] - font_layout[i];
                    if (SDL_RenderCopy(renderer, font, &SrcR, &DstR)) {
                        set_error("Failed to render text: %s", SDL_GetError());
                        return 0;
                    }
                    DstR.x += DstR.w;
                }
                else {
                    DstR.x += font_h/3; /* substitute for space */
                }
                line_start++;
            }

            /* Move to next line */
            line_y += font_h;
            line_start = text_p + 1;
            if (line_width > max_width) {
                max_width = line_width;
            }
            line_width = 0;
        }
        else {
            /* Calculate width of current line */
            if (*text_p != ' ') {
                i = *text_p - '!';
                line_width += font_layout[i+1] - font_layout[i];
            }
            else {
                line_width += font_h/3;
            }
        }
        text_p++;
    }

    /* Render final line if needed */
    if (line_start < text_p) {
        /* Calculate start x position for last line */
        if (horizontal_align == ALIGN_RIGHT) {
            DstR.x = x - line_width;
        }
        else if (horizontal_align == ALIGN_CENTER_HORIZONTAL) {
            DstR.x = x - line_width/2;
        }
        else { /* ALIGN_LEFT */
            DstR.x = x;
        }
        DstR.y = line_y;

        /* Render the last line */
        while(line_start < text_p) {
            if (*line_start != ' ') {
                i = *line_start - '!';
                SrcR.x = font_layout[i];
                SrcR.w = DstR.w = font_layout[i+1] - font_layout[i];
                if (SDL_RenderCopy(renderer, font, &SrcR, &DstR)) {
                    set_error("Failed to render text: %s", SDL_GetError());
                    return 0;
                }
                DstR.x += DstR.w;
            }
            else {
                DstR.x += font_h/3; /* substitute for space */
            }
            line_start++;
        }
        if (line_width > max_width) {
            max_width = line_width;
        }
    }

    return max_width;
}
