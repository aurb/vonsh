#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "types.h"
#include "error_handling.h"
#include "menu_logic.h"
#include "audio.h"
#include "config.h"
#include "hiscores.h"
#include "game_logic.h" // For start_play()

// Menu operational variables
static Menu *current_menu = NULL;
static int selected_item_index = 0;
static int main_menu_last_selected_index = 1;
static int active_entry_item_index = -1;
static char entry_buffer[10];
static int entry_buffer_len = 0;

// Title Screen Menu
MenuItem title_menu_items[] = {
    { .type = MenuItemType_Label, .label = "version: " VERSION_STR, .active = false, .action = NULL, .data.label_info = { .color = TEXT_GREY } },
    { .type = MenuItemType_Label, .label = "Play", .active = true, .action = menu_action_play, .data.label_info = { .color = TEXT_YELLOW } },
    { .type = MenuItemType_Label, .label = "Options", .active = true, .action = menu_action_go_to_options_menu, .data.label_info = { .color = TEXT_WHITE } },
    { .type = MenuItemType_Label, .label = "Hall of fame", .active = true, .action = menu_action_go_to_hall_of_fame, .data.label_info = { .color = TEXT_WHITE } },
    { .type = MenuItemType_Label, .label = "Exit", .active = true, .action = menu_action_exit, .data.label_info = { .color = TEXT_WHITE } }
};

Menu title_menu = {
    .items = title_menu_items,
    .count = sizeof(title_menu_items) / sizeof(MenuItem),
};

// Options Screen Menu
static const char* const g_switch_states[] = {"OFF", "ON"};
static const char* const g_display_states[] = {"window", "fullscreen"};

MenuItem options_menu_items[] = {
    { .type = MenuItemType_Label, .label = "Options", .active = false, .action = NULL, .data.label_info = { .color = TEXT_GREY } },
    { .type = MenuItemType_KeyConfig, .label = "Left:", .active = true, .action = menu_action_start_key_entry, .data.key_config_info = { .key_code = &g_game.key_left } },
    { .type = MenuItemType_KeyConfig, .label = "Right:", .active = true, .action = menu_action_start_key_entry, .data.key_config_info = { .key_code = &g_game.key_right } },
    { .type = MenuItemType_KeyConfig, .label = "Up:", .active = true, .action = menu_action_start_key_entry, .data.key_config_info = { .key_code = &g_game.key_up } },
    { .type = MenuItemType_KeyConfig, .label = "Down:", .active = true, .action = menu_action_start_key_entry, .data.key_config_info = { .key_code = &g_game.key_down } },
    { .type = MenuItemType_IntConfig, .label = "Board Width:", .active = true, .action = menu_action_start_int_entry, .data.int_config_info = { .value = &g_game.current_board_w, .min_value = BOARD_MIN_WIDTH } },
    { .type = MenuItemType_IntConfig, .label = "Board Height:", .active = true, .action = menu_action_start_int_entry, .data.int_config_info = { .value = &g_game.current_board_h, .min_value = BOARD_MIN_HEIGHT } },
    { .type = MenuItemType_Label, .label = "Pause: SPACE", .active = false, .action = NULL, .data.label_info = { .color = TEXT_GREY } },
    { .type = MenuItemType_Switch, .label = "Music:", .active = true, .action = menu_action_toggle_music, .data.switch_info = { .value = &g_game.music_on, .states = g_switch_states } },
    { .type = MenuItemType_Switch, .label = "Sound effects:", .active = true, .action = menu_action_toggle_sfx, .data.switch_info = { .value = &g_game.sfx_on, .states = g_switch_states } },
    { .type = MenuItemType_Switch, .label = "Display:", .active = true, .action = menu_action_toggle_fullscreen, .data.switch_info = { .value = &g_game.fullscreen, .states = g_display_states } },
    { .type = MenuItemType_Label, .label = "Back", .active = true, .action = menu_action_go_to_main_menu, .data.label_info = { .color = TEXT_YELLOW } }
};

Menu options_menu = {
    .items = options_menu_items,
    .count = sizeof(options_menu_items) / sizeof(MenuItem),
};

// Hall of Fame Menu
Table hall_of_fame_table;
static MenuItem* hall_of_fame_menu_items = NULL;
static Menu hall_of_fame_menu;

static void free_hall_of_fame_table_data() {
    if (hall_of_fame_table.rows) {
        for (int i = 0; i < hall_of_fame_table.num_rows; i++) {
            if (i > 0) {
                for (int j = 0; j < hall_of_fame_table.rows[i].num_cells; j++) {
                    free(hall_of_fame_table.rows[i].cells[j]);
                }
            }
            free(hall_of_fame_table.rows[i].cells);
        }
        free(hall_of_fame_table.rows);
        hall_of_fame_table.rows = NULL;
        hall_of_fame_table.num_rows = 0;
    }
}

static void free_hall_of_fame_menu_data() {
    free_hall_of_fame_table_data();
    if (hall_of_fame_menu_items) {
        free(hall_of_fame_menu_items);
        hall_of_fame_menu_items = NULL;
    }
}

static void build_hall_of_fame_table() {
    const Hiscore* scores = hiscores_get_scores();
    int score_count = 0;
    while (score_count < MAX_HISCORES && scores[score_count].score > 0) {
        score_count++;
    }

    if (score_count > 0) {
        int table_rows = score_count + 1;
        hall_of_fame_table.rows = malloc(table_rows * sizeof(TableRow));
        hall_of_fame_table.num_rows = table_rows;

        hall_of_fame_table.rows[0].cells = malloc(4 * sizeof(char*));
        hall_of_fame_table.rows[0].num_cells = 4;
        hall_of_fame_table.rows[0].cells[0] = "Player";
        hall_of_fame_table.rows[0].cells[1] = "Score";
        hall_of_fame_table.rows[0].cells[2] = "Date";
        hall_of_fame_table.rows[0].cells[3] = "Board";
        hall_of_fame_table.rows[0].color = TEXT_GREY;

        for (int i = 0; i < score_count; i++) {
            TableRow* row = &hall_of_fame_table.rows[i + 1];
            row->cells = malloc(4 * sizeof(char*));
            row->num_cells = 4;
            row->cells[0] = strdup(scores[i].player_name);

            row->cells[1] = malloc(12);
            sprintf(row->cells[1], "%d", scores[i].score);

            row->cells[2] = malloc(11);
            struct tm *tm_date = localtime(&scores[i].date);
            if (tm_date) {
                strftime(row->cells[2], 11, "%Y-%m-%d", tm_date);
            } else {
                strcpy(row->cells[2], "----------");
            }

            row->cells[3] = malloc(10);
            sprintf(row->cells[3], "%dx%d", scores[i].board_width, scores[i].board_height);

            row->color = TEXT_WHITE;
        }
    } else {
        hall_of_fame_table.num_rows = 0;
    }
}

static void build_hall_of_fame_menu() {
    free_hall_of_fame_menu_data();
    build_hall_of_fame_table();

    if (hall_of_fame_table.num_rows == 0) {
        hall_of_fame_menu.count = 3;
        hall_of_fame_menu_items = realloc(hall_of_fame_menu_items, hall_of_fame_menu.count * sizeof(MenuItem));
        hall_of_fame_menu_items[0] = (MenuItem){
            .type = MenuItemType_Label,
            .label = "Hall of fame",
            .active = false,
            .action = NULL,
            .data.label_info = { .color = TEXT_GREY }
        };
        hall_of_fame_menu_items[1] = (MenuItem){
            .type = MenuItemType_Label,
            .label = "The table is empty. Play!",
            .active = false,
            .action = NULL,
            .data.label_info = { .color = TEXT_GREY }
        };
        hall_of_fame_menu_items[2] = (MenuItem){
            .type = MenuItemType_Label,
            .label = "Back",
            .active = true,
            .action = menu_action_go_to_main_menu,
            .data.label_info = { .color = TEXT_YELLOW }
        };
        hall_of_fame_menu.items = hall_of_fame_menu_items;
    } else {
        int table_rows = hall_of_fame_table.num_rows;

        hall_of_fame_menu.count = table_rows + 3;
        hall_of_fame_menu_items = realloc(hall_of_fame_menu_items, hall_of_fame_menu.count * sizeof(MenuItem));

        hall_of_fame_menu_items[0] = (MenuItem){
            .type = MenuItemType_Label,
            .label = "Hall of fame",
            .active = false,
            .action = NULL,
            .data.label_info = { .color = TEXT_GREY }
        };

        for (int i = 0; i < table_rows; i++) {
            hall_of_fame_menu_items[i+1] = (MenuItem){
                .type = MenuItemType_TableRow,
                .active = false,
                .action = NULL,
                .data.table_row_info.row = &hall_of_fame_table.rows[i]
            };
        }

        hall_of_fame_menu_items[table_rows+1] = (MenuItem){
            .type = MenuItemType_Label,
            .label = "Clear scores",
            .active = (hall_of_fame_table.num_rows > 1),
            .action = menu_action_clear_scores,
            .data.label_info = { .color = TEXT_WHITE }
        };
        hall_of_fame_menu_items[table_rows + 2] = (MenuItem){
            .type = MenuItemType_Label,
            .label = "Back",
            .active = true,
            .action = menu_action_go_to_main_menu,
            .data.label_info = { .color = TEXT_YELLOW }
        };

        hall_of_fame_menu.items = hall_of_fame_menu_items;
    }
}

void menu_logic_init(void) {
    menu_action_go_to_main_menu(NULL);
}

static void handle_key_configuring_events(SDL_Event *event) {
    SDL_KeyCode pressed_key = event->key.keysym.sym;
    if (active_entry_item_index == -1) { return; } //redundant check
    //fetch currently active item
    MenuItem* item = &current_menu->items[active_entry_item_index];
    //we only allow to set the pressed key if it's not already used for some other setting
    //and if it's not Enter or Return key
    if (!(pressed_key == g_game.key_left || pressed_key == g_game.key_right || pressed_key == g_game.key_up || pressed_key == g_game.key_down || pressed_key == g_game.key_pause || pressed_key == SDLK_RETURN || pressed_key == SDLK_KP_ENTER)) {
        *(item->data.key_config_info.key_code) = pressed_key;
        save_user_config();
    }
    active_entry_item_index = -1;
    g_game.state = OptionsMenu;
}

static void handle_numeric_input_events(SDL_Event *event) {
    MenuItem* item = &current_menu->items[active_entry_item_index];
    SDL_Keycode sym = event->key.keysym.sym;

    if (sym >= SDLK_0 && sym <= SDLK_9) {
        if ((size_t)entry_buffer_len < sizeof(entry_buffer) - 1) {
            entry_buffer[entry_buffer_len++] = '0' + (sym - SDLK_0);
            entry_buffer[entry_buffer_len] = '\0';
        }
    } else if (sym == SDLK_BACKSPACE) {
        if (entry_buffer_len > 0) {
            entry_buffer[--entry_buffer_len] = '\0';
        }
    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
        if (entry_buffer_len > 0) {
            int new_value = atoi(entry_buffer);
            if (new_value >= item->data.int_config_info.min_value) {
                if (!g_game.fullscreen) {
                    **(item->data.int_config_info.value) = new_value;
                    g_game.window_w = (*g_game.current_board_w) * TILE_SIZE;
                    g_game.window_h = (*g_game.current_board_h + 1) * TILE_SIZE;
                    SDL_SetWindowSize(g_gfx.screen, g_game.window_w, g_game.window_h);
                    SDL_SetWindowPosition(g_gfx.screen, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    reinit_game_board_resources();
                    if (get_first_error()) return;
                    save_user_config();
                }
            }
        }
        entry_buffer[0] = '\0';
        g_game.state = OptionsMenu;
        active_entry_item_index = -1;
        entry_buffer_len = 0;
    }
}

void menu_logic_handle_event(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN && active_entry_item_index != -1) {
        if (event->key.keysym.sym == SDLK_ESCAPE) {
            active_entry_item_index = -1;
            entry_buffer_len = 0;
            entry_buffer[0] = '\0';
        }
        else if (current_menu->items[active_entry_item_index].type  == MenuItemType_KeyConfig) {
            handle_key_configuring_events(event);
        }
        else if (current_menu->items[active_entry_item_index].type  == MenuItemType_IntConfig) {
            handle_numeric_input_events(event);
        }
    }
    else {
        switch (event->type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    int mouse_x = event->button.x;
                    int mouse_y = event->button.y;
                    for (int i = 0; i < current_menu->count; i++) {
                        MenuItem* item = &current_menu->items[i];
                        if (mouse_x >= item->rect.x && mouse_x <= item->rect.x + item->rect.w &&
                            mouse_y >= item->rect.y && mouse_y <= item->rect.y + item->rect.h &&
                            item->action != NULL && item->active) {
                            selected_item_index = i;
                            //deactivate previous entry item if it's not the same as the selected item
                            if (active_entry_item_index != -1 && active_entry_item_index != selected_item_index) {
                                active_entry_item_index = -1;
                                entry_buffer_len = 0;
                                entry_buffer[0] = '\0';
                            }
                            if (item->type == MenuItemType_KeyConfig) {
                                menu_action_start_key_entry(item);
                            } else if (item->type == MenuItemType_IntConfig) {
                                menu_action_start_int_entry(item);
                            } else if (item->action) {
                                item->action(item);
                            }
                            break;
                        }
                    }
                }
                break;
            case SDL_KEYDOWN:
                switch (event->key.keysym.sym) {
                    case SDLK_UP:
                        do {
                            selected_item_index = (selected_item_index - 1 + current_menu->count) % current_menu->count;
                        } while (current_menu->items[selected_item_index].active == false);
                        break;
                    case SDLK_DOWN:
                        do {
                            selected_item_index = (selected_item_index + 1) % current_menu->count;
                        } while (current_menu->items[selected_item_index].active == false);
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                    case SDLK_SPACE:
                        if (g_game.state == OptionsMenu && current_menu->items[selected_item_index].type == MenuItemType_KeyConfig) {
                            active_entry_item_index = selected_item_index;
                            menu_action_start_key_entry(&current_menu->items[selected_item_index]);
                        } else if (g_game.state == OptionsMenu && current_menu->items[selected_item_index].type == MenuItemType_IntConfig) {
                            if (!g_game.fullscreen) {
                                menu_action_start_int_entry(&current_menu->items[selected_item_index]);
                            }
                        } else if (current_menu->items[selected_item_index].action != NULL) {
                            current_menu->items[selected_item_index].action(&current_menu->items[selected_item_index]);
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

void handle_game_over_events(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN || (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)) {
        main_menu_last_selected_index = 1;
        menu_action_go_to_main_menu(NULL);
    }
}

void handle_entering_hiscore_events(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER) {
            hiscores_add(g_game.player_name_len > 0 ? g_game.player_name : "Somebody", g_game.score, *g_game.current_board_w, *g_game.current_board_h);
            SDL_StopTextInput();
            main_menu_last_selected_index = 1;
            menu_action_go_to_main_menu(NULL);
        } else if (event->key.keysym.sym == SDLK_BACKSPACE && g_game.player_name_len > 0) {
            g_game.player_name_len--;
            g_game.player_name[g_game.player_name_len] = '\0';
        }
    } else if (event->type == SDL_TEXTINPUT) {
        if (g_game.player_name_len < MAX_NAME_LEN) {
            strncat(g_game.player_name, event->text.text, MAX_NAME_LEN - g_game.player_name_len);
            g_game.player_name_len = strlen(g_game.player_name);
        }
    }
}

void handle_escape_key(void) {
    if (active_entry_item_index == -1) { //process Esc key only if not in entry item
        if (g_game.state == OptionsMenu || g_game.state == HallOfFame) {
            menu_action_go_to_main_menu(NULL);
        } else if (g_game.state == GameOver || g_game.state == EnteringHiscoreName) {
            main_menu_last_selected_index = 1;
            menu_action_go_to_main_menu(NULL);
        } else {
            menu_action_exit(NULL);
        }
    }
}

void menu_action_play(MenuItem* item) {
    (void)item;
    current_menu = NULL;
    start_play();
}

void menu_action_go_to_main_menu(MenuItem* item) {
    (void)item;
    g_game.state = MainMenu;
    current_menu = &title_menu;
    selected_item_index = main_menu_last_selected_index;
    SDL_ShowCursor(SDL_ENABLE);
}

void menu_action_go_to_options_menu(MenuItem* item) {
    (void)item;
    main_menu_last_selected_index = selected_item_index;
    g_game.state = OptionsMenu;
    current_menu = &options_menu;
    menu_logic_update_board_size_items_state();
    selected_item_index = 0;
    while (current_menu->items[selected_item_index].active == false) {
        selected_item_index = (selected_item_index + 1) % current_menu->count;
    }
    SDL_ShowCursor(SDL_ENABLE);
}

void menu_action_go_to_hall_of_fame(MenuItem* item) {
    (void)item;
    main_menu_last_selected_index = selected_item_index;
    build_hall_of_fame_menu();
    g_game.state = HallOfFame;
    current_menu = &hall_of_fame_menu;
    selected_item_index = hall_of_fame_menu.count - 1;
    SDL_ShowCursor(SDL_ENABLE);
}

void menu_action_exit(MenuItem* item) {
    (void)item;
    g_game.state = NotInitialized;
    current_menu = NULL;
}

void menu_action_toggle_music(MenuItem* item) {
    (void)item;
    audio_toggle_music();
    save_user_config();
}

void menu_action_toggle_sfx(MenuItem* item) {
    (void)item;
    g_game.sfx_on = !g_game.sfx_on;
    save_user_config();
}

void menu_action_toggle_fullscreen(MenuItem* item) {
    (void)item;
    g_game.fullscreen = !g_game.fullscreen;

    if (g_game.fullscreen) {
        g_game.current_board_w = &g_game.fullscreen_board_w;
        g_game.current_board_h = &g_game.fullscreen_board_h;
        create_fullscreen_display();
    } else {
        g_game.current_board_w = &g_game.window_board_w;
        g_game.current_board_h = &g_game.window_board_h;
        create_windowed_display();
    }
    if (get_first_error()) return;
    
    reinit_game_board_resources();
    if (get_first_error()) return;
    menu_logic_update_board_size_items_state();
    save_user_config();
}

void menu_action_start_key_entry(MenuItem* item) {
    (void)item;
    active_entry_item_index = selected_item_index;
}

void menu_action_start_int_entry(MenuItem* item) {
    active_entry_item_index = selected_item_index;
    sprintf(entry_buffer, "%d", **item->data.int_config_info.value);
    entry_buffer_len = strlen(entry_buffer);
}

void menu_action_clear_scores(MenuItem* item) {
    (void)item;
    hiscores_clear();
    build_hall_of_fame_menu();
    selected_item_index = hall_of_fame_menu.count - 1;
}

void menu_logic_update_board_size_items_state(void) {
    for (int i = 0; i < options_menu.count; i++) {
        MenuItem* it = &options_menu.items[i];
        if (it->type == MenuItemType_IntConfig) {
            it->active = !g_game.fullscreen;
        }
    }
}

// Accessors
Menu* menu_logic_get_current_menu(void) {
    return current_menu;
}

int menu_logic_get_selected_item_index(void) {
    return selected_item_index;
}

int menu_logic_get_active_entry_item_index(void) {
    return active_entry_item_index;
}

const char* menu_logic_get_entry_buffer(void) {
    return entry_buffer;
}
