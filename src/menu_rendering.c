#include "types.h"
#include "error_handling.h"
#include "menu_rendering.h"
#include "menu_logic.h"
#include "text_renderer.h"
#include <math.h>


extern Table hall_of_fame_table; // Defined in menu_logic.c

static void calculate_scores_table_layout(int* col_widths, int* col_x) {
    int m_width = get_text_width("M");
    int zero_width = get_text_width("0");
    int player_w = 15 * m_width;
    int score_w = get_text_width("Score");
    int date_w = 10 * zero_width;
    int board_w = get_text_width("Board");
    int col_spacing = 1 * m_width;
    int edge_spacing = 2 * m_width;

    for (int i = 1; i < hall_of_fame_table.num_rows; i++) {
        int w = get_text_width(hall_of_fame_table.rows[i].cells[1]);
        if (w > score_w) score_w = w;
        w = get_text_width(hall_of_fame_table.rows[i].cells[3]);
        if (w > board_w) board_w = w;
    }

    col_widths[0] = player_w;
    col_widths[1] = score_w;
    col_widths[2] = date_w;
    col_widths[3] = board_w;

    int total_table_w = col_widths[0] + col_widths[1] + col_widths[2] + col_widths[3] + 3 * col_spacing + 2 * edge_spacing;
    int table_x = (g_game.window_w - total_table_w) / 2;

    col_x[0] = table_x + edge_spacing;
    col_x[1] = col_x[0] + player_w + col_spacing;
    col_x[2] = col_x[1] + score_w + col_spacing;
    col_x[3] = col_x[2] + date_w + col_spacing;
}

static int get_hall_of_fame_table_width() {
    int col_widths[4], col_x[4];
    calculate_scores_table_layout(col_widths, col_x);
    int m_width = get_text_width("M");
    int col_spacing = 1 * m_width;
    int edge_spacing = 2 * m_width;
    return col_widths[0] + col_widths[1] + col_widths[2] + col_widths[3] + 3 * col_spacing + 2 * edge_spacing;
}

static void render_menu_items() {
    Menu *menu = menu_logic_get_current_menu();
    int selected_item_index = menu_logic_get_selected_item_index();
    int active_entry_item_index = menu_logic_get_active_entry_item_index();
    const char* entry_buffer = menu_logic_get_entry_buffer();

    int current_y = g_game.window_h / 2 - (menu->count * MENU_ITEM_HEIGHT) / 2 + MENU_HEAD_SPACE;
    char text_buf[40];

    int col_widths[4], col_x[4];
    if (g_game.state == HallOfFame) {
        calculate_scores_table_layout(col_widths, col_x);
    }

    for (int i = 0; i < menu->count; i++) {
        MenuItem* item = &menu->items[i];
        TextColor color;

        if (item->type == MenuItemType_TableRow) {
            color = item->data.table_row_info.row->color;
        } else if (!item->active) {
            color = TEXT_GREY;
        } else if (item->type == MenuItemType_Label) {
            color = item->data.label_info.color;
        } else {
            color = TEXT_WHITE;
        }

        if (item->type == MenuItemType_TableRow) {
            TableRow* row = item->data.table_row_info.row;
            for (int j = 0; j < row->num_cells; j++) {
                render_text(g_gfx.renderer, g_gfx.txt_font, row->cells[j], col_x[j] + col_widths[j] / 2, current_y + MENU_ITEM_HEIGHT / 2, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, color);
                if (get_first_error()) return;
            }
        } else {
            switch (item->type) {
            case MenuItemType_KeyConfig: {
                const char* key_name = SDL_GetKeyName(*(item->data.key_config_info.key_code));
                if (active_entry_item_index == i) {
                    sprintf(text_buf, "%s ...", item->label);
                } else {
                    sprintf(text_buf, "%s %s", item->label, key_name);
                }
                break;
            }
            case MenuItemType_IntConfig: {
                if (active_entry_item_index == i) {
                    sprintf(text_buf, "%s %s_", item->label, entry_buffer);
                } else {
                    sprintf(text_buf, "%s %d", item->label, **item->data.int_config_info.value);
                }
                break;
            }
            case MenuItemType_Switch: {
                sprintf(text_buf, "%s %s", item->label, item->data.switch_info.states[*(item->data.switch_info.value) ? 1 : 0]);
                break;
            }
            case MenuItemType_Label:
            default: {
                sprintf(text_buf, "%s", item->label);
                break;
            }
            }

            item->rect.w = MENU_ITEM_WIDTH;
            item->rect.h = MENU_ITEM_HEIGHT;
            item->rect.x = g_game.window_w/2 - MENU_ITEM_WIDTH/2;
            item->rect.y = current_y;
            render_text(g_gfx.renderer, g_gfx.txt_font, text_buf, g_game.window_w/2, current_y + item->rect.h/2, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, color);
            if (get_first_error()) return;

            if (i == selected_item_index) {
                SDL_SetRenderDrawColor(g_gfx.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawRect(g_gfx.renderer, &item->rect);
            }
        }

        if (i == 0) {
            current_y += MENU_HEAD_SPACE;
        } else {
            current_y += MENU_ITEM_HEIGHT;
        }
    }
}

void render_menus(void) {
    int x,y;
    Menu *menu = menu_logic_get_current_menu();

    SDL_Rect DstR_bg = { 0, 0, TILE_SIZE, TILE_SIZE };
    float bg_time = g_game.frame / 80.0f;
    for (y=0; y < g_game.window_h / TILE_SIZE +1; y++) {
        for (x=0; x < g_game.window_w / TILE_SIZE +1; x++) {
            DstR_bg.x = x*TILE_SIZE; DstR_bg.y = y*TILE_SIZE;
            float val = sinf(x * 0.0666f + bg_time + sinf(y * 0.3f + bg_time)) + sinf( y * 0.0666f + sinf(x * 0.35f + bg_time));
            int tile_index = (int)((val + 2.0f) / 4.0f * (GROUND_TILES - 1) + 0.5f);
            if (tile_index < 0) tile_index = 0;
            if (tile_index >= GROUND_TILES) tile_index = GROUND_TILES - 1;
            SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_env_tileset, &g_gfx.ground_tile[tile_index], &DstR_bg);
        }
    }

    SDL_Rect dark_rect;
    if (g_game.state == HallOfFame) {
        dark_rect.w = get_hall_of_fame_table_width();
    } else {
        dark_rect.w = MENU_WIDTH;
    }
    dark_rect.x = (g_game.window_w - dark_rect.w) / 2;

    SDL_Rect logo_dst;
    SDL_QueryTexture(g_gfx.txt_logo, NULL, NULL, &logo_dst.w, &logo_dst.h);
    
    dark_rect.h = 2*MENU_BORDER + logo_dst.h + MENU_LOGO_SPACE + MENU_HEAD_SPACE + (menu->count-1) * MENU_ITEM_HEIGHT;
    dark_rect.y = (g_game.window_h - dark_rect.h) / 2;
    
    SDL_SetRenderDrawBlendMode(g_gfx.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_gfx.renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(g_gfx.renderer, &dark_rect);
    SDL_SetRenderDrawBlendMode(g_gfx.renderer, SDL_BLENDMODE_NONE);

    logo_dst.x = g_game.window_w/2 - logo_dst.w/2;
    logo_dst.y = dark_rect.y + MENU_BORDER;
    SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_logo, NULL, &logo_dst);

    render_menu_items();
}

static int render_game_over_scores_table(int yc) {
    int col_widths[4], col_x[4];
    calculate_scores_table_layout(col_widths, col_x);

    for (int i = 0; i < hall_of_fame_table.num_rows; i++) {
        TableRow* row = &hall_of_fame_table.rows[i];
        for (int j = 0; j < row->num_cells; j++) {
            render_text(g_gfx.renderer, g_gfx.txt_font, row->cells[j], col_x[j] + col_widths[j] / 2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, row->color);
            if (get_first_error()) return yc;
        }
        yc += MENU_ITEM_HEIGHT;
    }
    return yc;
}

void render_game_over_overlay(void) {
    int l;
    SDL_Rect DstR;

    int text_height = get_text_height("X");
    SDL_SetRenderDrawColor(g_gfx.renderer, 0, 0, 0, 127);
    SDL_QueryTexture(g_gfx.txt_trophy, NULL, NULL, &l, &l);
    
    int items_count = 0;
    if (g_game.state == EnteringHiscoreName) {
        DstR.w = GAME_OVER_WIDTH;
        if (g_game.new_record) {
            items_count = 5;
        }
        else {
            items_count = 3;
        }
    }
    else {
        if (hall_of_fame_table.num_rows > 0) {
            DstR.w = get_hall_of_fame_table_width();
        } else {
            DstR.w = GAME_OVER_WIDTH;
        }
        items_count = 2;
        if (hall_of_fame_table.num_rows > 0) {
            items_count += hall_of_fame_table.num_rows;
        }
    }
    DstR.h = 2 * GAME_OVER_BORDER + (items_count - 1)*MENU_ITEM_HEIGHT + text_height;
    if (g_game.new_record) {
        DstR.h += l - text_height;
    }

    int yc = (g_game.window_h-TILE_SIZE-DstR.h)/2;
    DstR.x = (g_game.window_w-DstR.w)/2; DstR.y = yc;
    SDL_RenderFillRect(g_gfx.renderer, &DstR);
    yc += GAME_OVER_BORDER + text_height/2;
    render_text(g_gfx.renderer, g_gfx.txt_font, "Game Over", g_game.window_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, TEXT_WHITE);
    if (get_first_error()) return;
    yc += MENU_ITEM_HEIGHT;

    if (g_game.new_record) {
        DstR.w = l; DstR.h = l;
        DstR.x = (g_game.window_w-DstR.w)/2; DstR.y = yc - text_height/2;
        SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_trophy, NULL, &DstR);
        yc += l + MENU_ITEM_HEIGHT - text_height;

        if (g_game.frame % 15 < 10) {
            render_text(g_gfx.renderer, g_gfx.txt_font, "NEW HIGH SCORE !", g_game.window_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, TEXT_YELLOW);
            if (get_first_error()) return;
        }
        yc += MENU_ITEM_HEIGHT;
    }

    if (g_game.state == EnteringHiscoreName) {
        char text_buf[40];
        sprintf(text_buf, "Enter name:");
        render_text(g_gfx.renderer, g_gfx.txt_font, text_buf, g_game.window_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, TEXT_YELLOW);
        yc += MENU_ITEM_HEIGHT;
        sprintf(text_buf, "%s_", g_game.player_name);
        render_text(g_gfx.renderer, g_gfx.txt_font, text_buf, g_game.window_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, TEXT_WHITE);
    } else {
        if (hall_of_fame_table.num_rows > 0) {
            yc = render_game_over_scores_table(yc);
            if (get_first_error()) return;
        }
        render_text(g_gfx.renderer, g_gfx.txt_font, "Press any key", g_game.window_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL, TEXT_YELLOW);
    }
    if (get_first_error()) return;
}
