#ifndef MENU_LOGIC_H
#define MENU_LOGIC_H

#include <SDL2/SDL.h>
#include "types.h"

void menu_logic_init(void);
void menu_logic_handle_event(SDL_Event *event);
Menu* menu_logic_get_current_menu(void);
int menu_logic_get_selected_item_index(void);
int menu_logic_get_active_entry_item_index(void);
const char* menu_logic_get_entry_buffer(void);
void menu_logic_update_board_size_items_state(void);

// Menu actions
void menu_action_play(MenuItem* item);
void menu_action_go_to_main_menu(MenuItem* item);
void menu_action_go_to_options_menu(MenuItem* item);
void menu_action_exit(MenuItem* item);
void menu_action_toggle_music(MenuItem* item);
void menu_action_toggle_sfx(MenuItem* item);
void menu_action_toggle_fullscreen(MenuItem* item);
void menu_action_start_key_entry(MenuItem* item);
void menu_action_start_int_entry(MenuItem* item);
void menu_action_go_to_hall_of_fame(MenuItem* item);
void menu_action_clear_scores(MenuItem* item);
void handle_escape_key(void);
void handle_entering_hiscore_events(SDL_Event *event);
void handle_game_over_events(SDL_Event *event);

#endif // MENU_LOGIC_H
