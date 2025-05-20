#include <unistd.h>
#include <stdbool.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <cjson/cJSON.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include "types.h"
#include "error_handling.h"
#include "text_renderer.h"
#include "pcg_basic.h"
#include "hiscores.h"
#include "audio.h"
#include "config.h"
#include "file_io.h"
#include "menu_logic.h"
#include "menu_rendering.h"
#include "game_logic.h"
#include "game_rendering.h"

#define CHECK_SDL_CALL(func_call, error_msg) \
    do { \
        if ((func_call) != 0) { \
            set_error(error_msg ": %s", SDL_GetError()); \
            return; \
        } \
    } while(0)

#define CHECK_SDL_PTR(ptr, error_msg) \
    do { \
        if ((ptr) == NULL) { \
            set_error(error_msg ": %s", SDL_GetError()); \
            return; \
        } \
    } while(0)


Graphics g_gfx = {0};
Game g_game = {0};

/* Callback for animation frame timer.
   Generates SDL_USEREVENT when main game loop shall render next frame. */
uint32_t tick_callback(uint32_t interval, void *param) {
    (void)param;
    SDL_Event event;
    SDL_UserEvent userevent;
    userevent.type = SDL_USEREVENT;  userevent.code = 0;
    userevent.data1 = NULL;  userevent.data2 = NULL;
    event.type = SDL_USEREVENT;  event.user = userevent;
    SDL_PushEvent(&event);
    return interval;
}

/*
    Initialize game engine
 */
static void init_game_engine() {
    /* Executed only at start of program */
    if (g_game.state != NotInitialized) {
        // This case should not happen in normal flow, but as a safeguard:
        set_error("Error: init_game_engine called in wrong state.");
        return;
    }

    /* init SDL library */
    CHECK_SDL_CALL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO), "Error initializing SDL");
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        set_error("Error initializing SDL_image: %s.", IMG_GetError());
        return;
    }

    #define EXE_PATH_SIZE (200)
    char exe_path[EXE_PATH_SIZE]; /* resources path */
    memset(exe_path, 0, EXE_PATH_SIZE);
    if (readlink("/proc/self/exe", exe_path, EXE_PATH_SIZE) == -1) {
        set_error("Work dir init error: Error reading executable path.");
        return;
    }
    if (chdir(dirname(exe_path)) == -1) {
        set_error("Work dir init error: Error setting current working directory.");
        return;
    }
    pcg32_srandom(time(NULL), 0x12345678); /* seed random number generator */

    /* Ensure user config exists and load it before creating window (affects window/fullscreen, keys, etc.) */
    ensure_user_config_exists();
    load_user_config();
    audio_init();
    if (get_first_error()) {
        return;
    }

    /* Create window and renderer */
    Uint32 window_flags = g_game.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    g_gfx.screen = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                 SDL_WINDOWPOS_CENTERED,
                 g_game.fullscreen ? 0 : g_game.window_board_w * TILE_SIZE,
                 g_game.fullscreen ? 0 : (g_game.window_board_h + 1) * TILE_SIZE,
                 window_flags);
    CHECK_SDL_PTR(g_gfx.screen, "SDL window not created");
    g_game.fullscreen_board_w = -1;
    g_game.fullscreen_board_h = -1;

    /* Create display area based on mode */
    if (g_game.fullscreen) {
        create_fullscreen_display();
        if (get_first_error()) return;
    } else {
        create_windowed_display();
        if (get_first_error()) return;
    }

    g_gfx.renderer = SDL_CreateRenderer(g_gfx.screen, -1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    CHECK_SDL_PTR(g_gfx.renderer, "SDL renderer not created");

    /* load textures */
    load_texture(&g_gfx.txt_env_tileset, RES_DIR"board_tiles.png");
    if (get_first_error()) return;
    load_texture(&g_gfx.txt_food_tileset, RES_DIR"food_tiles.png");
    if (get_first_error()) return;
    load_texture(&g_gfx.txt_food_marker, RES_DIR"food_marker.png");
    if (get_first_error()) return;
    load_texture(&g_gfx.txt_char_tileset, RES_DIR"character_tiles.png");
    if (get_first_error()) return;
    load_texture(&g_gfx.txt_font, RES_DIR"good_neighbors.png");
    if (get_first_error()) return;
    load_texture(&g_gfx.txt_logo, RES_DIR"logo.png");
    if (get_first_error()) return;
    load_texture(&g_gfx.txt_trophy, RES_DIR"trophy-bronze.png");
    if (get_first_error()) return;

    /* set blending mode for food marker*/
    CHECK_SDL_CALL(SDL_SetTextureBlendMode(g_gfx.txt_food_marker, SDL_BLENDMODE_ADD), "Failed to set texture blend mode");

    /* allocate tile info arrays */
    if ((g_gfx.ground_tile = calloc(GROUND_TILES, sizeof(SDL_Rect))) == NULL ||
        (g_gfx.wall_tile = calloc(WALL_TILES, sizeof(SDL_Rect))) == NULL ||
        (g_gfx.food_tile = calloc(FOOD_TILES, sizeof(SDL_Rect))) == NULL) {
        set_error("Error: Error allocating memory for tile data.");
        return;
    }
    /* fill tile info arrays */
    {
        SDL_Rect ground_tile_temp[GROUND_TILES] = {
            {0 * TILE_SIZE, 0 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {1 * TILE_SIZE, 0 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {2 * TILE_SIZE, 0 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {3 * TILE_SIZE, 0 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {4 * TILE_SIZE, 0 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {0 * TILE_SIZE, 1 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {0 * TILE_SIZE, 2 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {0 * TILE_SIZE, 3 * TILE_SIZE, TILE_SIZE, TILE_SIZE}
        };
        for (int i = 0; i < GROUND_TILES; i++) {
            g_gfx.ground_tile[i] = ground_tile_temp[i];
        }
        SDL_Rect wall_tile_temp[WALL_TILES] = {
            {0 * TILE_SIZE, 17 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {1 * TILE_SIZE, 17 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {2 * TILE_SIZE, 17 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {3 * TILE_SIZE, 17 * TILE_SIZE, TILE_SIZE, TILE_SIZE}
        };
        for (int i = 0; i < WALL_TILES; i++) {
            g_gfx.wall_tile[i] = wall_tile_temp[i];
        }
        SDL_Rect food_tile_temp[FOOD_TILES] = {
            {4 * TILE_SIZE, 1 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {0 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {1 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {2 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {0 * TILE_SIZE, 6 * TILE_SIZE, TILE_SIZE, TILE_SIZE},
            {3 * TILE_SIZE, 6 * TILE_SIZE, TILE_SIZE, TILE_SIZE}
        };
        for (int i = 0; i < FOOD_TILES; i++) {
            g_gfx.food_tile[i] = food_tile_temp[i];
        }
    }

    /* init text rendering functionality */
    init_text_renderer(g_gfx.renderer, g_gfx.txt_font);
    /* Reset game board and generate new background texture. */
    reinit_game_board_resources();
    if (get_first_error()) return;

    /* Ensure cursor is visible initially */
    SDL_ShowCursor(SDL_ENABLE);
    
    menu_logic_init();
}

void cleanup_game(void) {
    audio_shutdown();

    if (g_game.game_timer) SDL_RemoveTimer(g_game.game_timer);

    if (g_gfx.txt_trophy) SDL_DestroyTexture(g_gfx.txt_trophy);
    if (g_gfx.txt_logo) SDL_DestroyTexture(g_gfx.txt_logo);
    if (g_gfx.txt_font) SDL_DestroyTexture(g_gfx.txt_font);
    if (g_gfx.txt_char_tileset) SDL_DestroyTexture(g_gfx.txt_char_tileset);
    if (g_gfx.txt_env_tileset) SDL_DestroyTexture(g_gfx.txt_env_tileset);
    if (g_gfx.txt_food_tileset) SDL_DestroyTexture(g_gfx.txt_food_tileset);
    if (g_gfx.txt_food_marker) SDL_DestroyTexture(g_gfx.txt_food_marker);
    if (g_gfx.txt_game_board) SDL_DestroyTexture(g_gfx.txt_game_board);

    if (g_gfx.renderer) SDL_DestroyRenderer(g_gfx.renderer);
    if (g_gfx.screen) SDL_DestroyWindow(g_gfx.screen);
    IMG_Quit();
    SDL_Quit();

    free(g_gfx.food_tile);
    free(g_gfx.wall_tile);
    free(g_gfx.ground_tile);
    free(g_game.game_board);
}

/* renders whole game state and blits everything to screen */
void display_screen(void)
{
    if (g_game.state == MainMenu || g_game.state == OptionsMenu || g_game.state == HallOfFame) {
        /* clear screen with black */
        CHECK_SDL_CALL(SDL_SetRenderDrawColor(g_gfx.renderer, 0,0,0, SDL_ALPHA_OPAQUE), "Failed to set render draw color");
        CHECK_SDL_CALL(SDL_RenderClear(g_gfx.renderer), "Failed to clear renderer");
        render_menus();
        if (g_game.fps_counter_on) {
            char fps_text[16];
            snprintf(fps_text, sizeof(fps_text), "FPS: %d", g_game.fps);
            render_text(g_gfx.renderer, g_gfx.txt_font, fps_text, g_game.window_w / 2, g_game.window_h - 1, ALIGN_CENTER_HORIZONTAL, ALIGN_BOTTOM, TEXT_WHITE);
        }
        SDL_RenderPresent(g_gfx.renderer);
    }
    else if (g_game.state == Playing || g_game.state == Paused || g_game.state == GameOver || g_game.state == EnteringHiscoreName) {
        /* clear screen with black */
        CHECK_SDL_CALL(SDL_SetRenderDrawColor(g_gfx.renderer, 0,0,0, SDL_ALPHA_OPAQUE), "Failed to set render draw color");
        CHECK_SDL_CALL(SDL_RenderClear(g_gfx.renderer), "Failed to clear renderer");
        render_game_view();
        if (g_game.fps_counter_on) {
            char fps_text[16];
            snprintf(fps_text, sizeof(fps_text), "FPS: %d", g_game.fps);
            render_text(g_gfx.renderer, g_gfx.txt_font, fps_text, g_game.window_w / 2, g_game.window_h - 1, ALIGN_CENTER_HORIZONTAL, ALIGN_BOTTOM, TEXT_WHITE);
        }
        SDL_RenderPresent(g_gfx.renderer);
    }
}

/*############ MAIN GAME LOOP #############*/
int main(int argc, char ** argv)
{
    time_t t;
    SDL_Event event;
    g_game.state = NotInitialized;
    int frame_count = 0;
    uint32_t fps_timer_start = SDL_GetTicks();
 
    if (argc > 1 && strcmp(argv[1], "fps") == 0) {
        g_game.fps_counter_on = true; //enable optional FPS counter
    }
    srand((unsigned) time(&t));
    init_game_engine();
    hiscores_init();
    // Error is set by init_game_engine, which also sets state to NotInitialized

    if (get_first_error() == NULL) {
        audio_play_idle_music();
    }
    /* Start timer for generating SDL_USEREVENT events for screen frame rendering */
    if (get_first_error() == NULL) {
        g_game.game_timer = SDL_AddTimer(RENDER_INTERVAL, tick_callback, 0);
        if (g_game.game_timer == 0) {
            set_error("Failed to create game timer: %s.", SDL_GetError());
        }
    }

    while (g_game.state != NotInitialized && get_first_error() == NULL) { /* main game loop */
        while (SDL_PollEvent(&event) && get_first_error() == NULL) { /* process pending events */
            switch (event.type) {
                case SDL_QUIT: /* window closed */
                    g_game.state = NotInitialized;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        handle_escape_key();
                    }
                    break;
                default: /* handle state-specific events */
                    break;
            }
            if (g_game.state == NotInitialized) {
                break;
            }

            switch (g_game.state) {
                case MainMenu:
                case OptionsMenu:
                case HallOfFame:
                    menu_logic_handle_event(&event);
                    break;
                case Playing:
                    handle_playing_events(&event);
                    break;
                case Paused:
                    handle_paused_events(&event);
                    break;
                case GameOver:
                    handle_game_over_events(&event);
                    break;
                case EnteringHiscoreName:
                    handle_entering_hiscore_events(&event);
                    break;
                default:
                    break;
            }
            if (event.type == SDL_USEREVENT) {
                // Always render, regardless of state, to show menus or game
                if (g_game.state == Playing && g_game.animation_progress == 0.0f) {
                    update_play_state();
                }
                display_screen();
                if (g_game.fps_counter_on) {
                    frame_count++;
                    uint32_t fps_timer_current = SDL_GetTicks();
                    if (fps_timer_current > fps_timer_start + FPS_COUNT_INTERVAL) {
                        g_game.fps = (int)((double)(frame_count*1000.)/(double)(fps_timer_current - fps_timer_start)+0.5);
                        frame_count = 0;
                        fps_timer_start = fps_timer_current;
                    }
                }
                if (get_first_error()) break;
                g_game.frame++;
                if (g_game.state == Playing) {
                    g_game.animation_progress = (g_game.frame % CHAR_ANIM_FRAMES)/(double)CHAR_ANIM_FRAMES;
                }

            }
        }
    }

    if (get_first_error() && g_gfx.screen == NULL) {
        // If the window wasn't even created, we can't use the renderer.
        // Also, we must initialize video just for the message box.
        SDL_Init(SDL_INIT_VIDEO);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vonsh Error", get_first_error(), NULL);
    } else if (get_first_error()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vonsh Error", get_first_error(), g_gfx.screen);
    }

    cleanup_game();

    return get_first_error() ? 1 : 0;
}

