#include "types.h"
#include "error_handling.h"
#include "game_rendering.h"
#include "text_renderer.h"
#include "menu_rendering.h"

// Helper function to get a pointer to a board field based on its coordinates
static inline BoardField* get_board_field(int x, int y) {
    return &g_game.game_board[(*g_game.current_board_w) * y + x];
}

void render_game_view(void) {
    int x, y;
    char txt_buf[40];

    SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_game_board, NULL, NULL);

    SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    SDL_Rect SrcR = { 0, 0, TILE_SIZE, TILE_SIZE };
    static const int dir_to_col[3][3] = {
        {-1, 3, -1},
        {2, -1, 0},
        {-1, 1, -1},
    };

    for (y=0; y<(*g_game.current_board_h); y++) {
        for (x=0; x<(*g_game.current_board_w); x++) {
            BoardField* field = get_board_field(x, y);
            switch(field->type) {
                case Empty:
                case Wall:
                    break;
                case Snake:
                    SrcR.x = dir_to_col[-field->pdx+1][-field->pdy+1] * TILE_SIZE;
                    SrcR.y = field->p*(CHAR_ANIM_FRAMES-1)*(TILE_SIZE+1) + 1;
                    int anim_frame = (int)(g_game.animation_progress * CHAR_ANIM_FRAMES);
                    if (anim_frame == 1) {
                        SrcR.y += (TILE_SIZE+1);
                    }
                    else if (anim_frame == 3) {
                        SrcR.y += 2*(TILE_SIZE+1);
                    }
                    DstR.x = x*TILE_SIZE;
                    DstR.y = y*TILE_SIZE;
                    DstR.x += (1.0-g_game.animation_progress) * field->pdx * TILE_SIZE;
                    DstR.y += (1.0-g_game.animation_progress) * field->pdy * TILE_SIZE;
                    SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_char_tileset, &SrcR, &DstR);
                    break;
                case Food:
                    DstR.x = x*TILE_SIZE;
                    DstR.y = y*TILE_SIZE;
                    if (g_game.state == Playing && g_game.frame % (FOOD_BLINK_FRAMES*3) < FOOD_BLINK_FRAMES) {
                        if ((g_game.frame/3) % 3 == 0) {
                            SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_food_marker, NULL, &DstR);
                        }
                        else if ((g_game.frame/3) % 3 == 1) {
                            SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_food_tileset, &g_gfx.food_tile[field->p], &DstR);
                        }
                    }
                    else {
                        SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_food_tileset, &g_gfx.food_tile[field->p], &DstR);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    SDL_SetRenderDrawBlendMode(g_gfx.renderer, SDL_BLENDMODE_BLEND);
    switch (g_game.state) {
        case EnteringHiscoreName:
        case GameOver:
            render_game_over_overlay();
            break;
        case Paused:
            render_text(g_gfx.renderer, g_gfx.txt_font, "PAUSED", g_game.window_w/2, g_game.window_h/2, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, TEXT_WHITE);
            break;
        default:
            break;
    }

    sprintf(txt_buf, "HIGH SCORE: %d", g_game.hi_score);
    render_text(g_gfx.renderer, g_gfx.txt_font, txt_buf, TILE_SIZE, (*g_game.current_board_h)*TILE_SIZE, ALIGN_LEFT, ALIGN_TOP, TEXT_WHITE);
    if (get_first_error()) return;
    sprintf(txt_buf, "SCORE: %d", g_game.score);
    render_text(g_gfx.renderer, g_gfx.txt_font, txt_buf, g_game.window_w-TILE_SIZE, (*g_game.current_board_h)*TILE_SIZE, ALIGN_RIGHT, ALIGN_TOP, TEXT_WHITE);
}
