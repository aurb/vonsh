#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <SDL2/SDL.h>

void start_play(void);
void pause_play(void);
void resume_play(void);
void update_play_state(void);
void handle_playing_events(SDL_Event *event);
void handle_paused_events(SDL_Event *event);
void switch_to_game_over(void);
// Board utilities, to be used by other modules
void reinit_game_board_resources(void);
void create_windowed_display(void);
void create_fullscreen_display(void);

#endif // GAME_LOGIC_H
