#include "types.h"
#include "game_logic.h"
#include "audio.h"
#include "error_handling.h"
#include "pcg_basic.h"
#include "hiscores.h"
// Helper function to get a pointer to a board field based on its coordinates
static inline BoardField* get_board_field(int x, int y) {
    return &g_game.game_board[(*g_game.current_board_w) * y + x];
}

/*
 * Fills game board with initial content(grass) and pre-renders it to a texture
 */
static void init_game_board_content(void) {
    /* Reset the game board array */
    for (int y = 0; y < *g_game.current_board_h; y++) {
        for (int x = 0; x < *g_game.current_board_w; x++) {
            BoardField* field = get_board_field(x, y);
            field->type = Empty;
            field->pdx = 0;
            field->pdy = 0;
        }
    }

    /* Redirect rendering to the game board texture */
    SDL_SetRenderTarget(g_gfx.renderer, g_gfx.txt_game_board);
    /* Fill render target with black. Later status bar at the bottom of the screen
       will have that color. */
    SDL_SetRenderDrawColor(g_gfx.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_gfx.renderer);

    /* Fill background with random pattern of ground tiles */
    int x, y;
    SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    for (y=0; y<*g_game.current_board_h; y++)
        for (x=0; x<*g_game.current_board_w; x++) {
            DstR.x = x*TILE_SIZE; DstR.y = y*TILE_SIZE;
            SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_env_tileset, &g_gfx.ground_tile[pcg32_boundedrand(8)], &DstR);
        }

    /* Detach the game board texture from renderer */
    SDL_SetRenderTarget(g_gfx.renderer, NULL);
}

// (Re)initialize board-dependent resources
void reinit_game_board_resources() {
    if (g_gfx.txt_game_board != NULL) {
        SDL_DestroyTexture(g_gfx.txt_game_board);
        g_gfx.txt_game_board = NULL;
    }
    if (g_game.game_board != NULL) {
        free(g_game.game_board);
        g_game.game_board = NULL;
    }

    g_gfx.txt_game_board = SDL_CreateTexture(g_gfx.renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_TARGET, g_game.window_w, g_game.window_h);
    if(g_gfx.txt_game_board == NULL) {
        set_error("Error creating background texture: %s", SDL_GetError());
        return;
    }

    g_game.game_board = calloc((*g_game.current_board_w) * (*g_game.current_board_h), sizeof(BoardField));
    if (g_game.game_board == NULL) {
        SDL_DestroyTexture(g_gfx.txt_game_board);
        g_gfx.txt_game_board = NULL;
        set_error("Error: Error allocating memory for game board.");
        return;
    }

    init_game_board_content();
}

/* Creates display area in windowed mode based on current game board dimensions */
void create_windowed_display(void) {
    g_game.window_w = g_game.window_board_w*TILE_SIZE;
    g_game.window_h = (g_game.window_board_h+1)*TILE_SIZE;
    g_game.current_board_w = &g_game.window_board_w;
    g_game.current_board_h = &g_game.window_board_h;

    if (g_game.state != NotInitialized) {
        SDL_SetWindowFullscreen(g_gfx.screen, 0);
        SDL_SetWindowSize(g_gfx.screen, g_game.window_w, g_game.window_h);
        SDL_SetWindowPosition(g_gfx.screen, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

/* Creates display area in fullscreen mode and updates game board dimensions */
void create_fullscreen_display(void) {
    if (g_game.state != NotInitialized) {
        SDL_SetWindowFullscreen(g_gfx.screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    
    SDL_GetWindowSize(g_gfx.screen, &g_game.window_w, &g_game.window_h);
    g_game.fullscreen_board_w = g_game.window_w/TILE_SIZE;
    g_game.fullscreen_board_h = g_game.window_h/TILE_SIZE - 1;
    g_game.current_board_w = &g_game.fullscreen_board_w;
    g_game.current_board_h = &g_game.fullscreen_board_h;
}

/* Adds obstacle or food at random location in game board. */
static bool seed_item(FieldType item_type, int *out_x, int *out_y)
{
    int x = 0, y = 0;
    int attempts = 0;
    BoardField *field = NULL;
    while(attempts < (*g_game.current_board_w) * (*g_game.current_board_h)) {
        x = pcg32_boundedrand(*g_game.current_board_w);
        y = pcg32_boundedrand(*g_game.current_board_h);
        field = get_board_field(x, y);
        if (field->type == Empty) {
            field->type = item_type;
            if (item_type == Food) {
                field->p = pcg32_boundedrand(6);
            }
            if (out_x) *out_x = x;
            if (out_y) *out_y = y;
            return true;
        }
        attempts++;
    }
    return false;
}

/* Set game state to GameOver and show cursor */
void switch_to_game_over(void) {
    if (hiscores_is_highscore(g_game.score)) {
        g_game.state = EnteringHiscoreName;
        g_game.new_record = g_game.score > hiscores_get_scores()[0].score;
        g_game.player_name[0] = '\0';
        g_game.player_name_len = 0;
        SDL_StartTextInput();
    } else {
        g_game.state = GameOver;
        g_game.new_record = false;
    }
    g_game.animation_progress = 1.0f;

    if (g_game.sfx_on) {
        audio_play_die_sound();
    }
    audio_play_idle_music();
    SDL_ShowCursor(SDL_ENABLE);
}

void update_play_state(void)
{
    BoardField* prev_field = NULL, *next_field = NULL;
    int next_hx = g_game.hx + g_game.dhx;
    int next_hy = g_game.hy + g_game.dhy;
    int px = g_game.hx, py = g_game.hy;
    int nx = next_hx, ny = next_hy;

    if (next_hx < 0 || next_hy < 0 ||
        next_hx >= *g_game.current_board_w || next_hy >= *g_game.current_board_h) {
        switch_to_game_over();
    }
    else {
        prev_field = get_board_field(px, py);
        next_field = get_board_field(nx, ny);
        if (next_field->type == Food) {
            g_game.score++;
            if (g_game.score > g_game.hi_score) {
                g_game.hi_score = g_game.score;
            }
            g_game.expand_counter += g_game.score;
            seed_item(Food, NULL, NULL);
            if (get_first_error()) return;
        }
        else if (next_field->type != Empty) {
            switch_to_game_over();
        }
    }

    if (g_game.state != Playing) return;

    //obtain tail position
    next_field->type = Snake;
    //set the vector from new head position to (possibly) the next head position
    next_field->pdx = -g_game.dhx;
    next_field->pdy = -g_game.dhy;

    px = nx; py=ny;
    //traverse the snake from head(next_field) to tail(prev_field)
    while(px!=g_game.tx || py!=g_game.ty) {
        nx = px;
        ny = py;
        px = px + next_field->pdx;
        py = py + next_field->pdy;
        prev_field = get_board_field(px, py);
        next_field->p = prev_field->p;
        next_field = prev_field;
    };

    if (g_game.expand_counter==0) {
        //if not expanding, reset the tail field content...
        next_field->type = Empty;
        next_field->p = 0;
        next_field->pdx = 0;
        next_field->pdy = 0;
        //...and set tail coordinates to new position
        g_game.tx = nx;
        g_game.ty = ny;
    }
    else {
        //if expanding, set the tail field content to a random character and seed a wall
        next_field->p = pcg32_boundedrand(TOTAL_CHARS);
        int wall_x, wall_y;
        if (seed_item(Wall, &wall_x, &wall_y)) {
            SDL_Rect DstR = { wall_x * TILE_SIZE, wall_y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
            SDL_SetRenderTarget(g_gfx.renderer, g_gfx.txt_game_board);
            SDL_RenderCopy(g_gfx.renderer, g_gfx.txt_env_tileset, &g_gfx.wall_tile[pcg32_boundedrand(4)], &DstR);
            SDL_SetRenderTarget(g_gfx.renderer, NULL);
        }
        if (get_first_error()) return;
        g_game.expand_counter--;
        if (g_game.sfx_on) {
            audio_play_exp_sound();
            if (get_first_error()) return;
        }
    }

    //calculate new head position
    g_game.hx = next_hx;
    g_game.hy = next_hy;

    g_game.ck_press = 0;
}

void start_play(void) {
    init_game_board_content();
    if (get_first_error()) return;

    g_game.dhx = 0;   g_game.dhy = -1;
    g_game.score = g_game.expand_counter = 0;
    g_game.hi_score = hiscores_get_scores()[0].score;
    g_game.hx = g_game.tx = (*g_game.current_board_w)/2;
    g_game.hy = g_game.ty = (*g_game.current_board_h)/2;

    BoardField* field = get_board_field(g_game.hx, g_game.hy);
    field->type = Snake;
    field->p = pcg32_boundedrand(TOTAL_CHARS);
    field->pdx = -g_game.dhx;
    field->pdy = -g_game.dhy;
    seed_item(Food, NULL, NULL);
    if (get_first_error()) return;
    g_game.ck_press = 0;
    g_game.frame = 0;
    g_game.animation_progress = 0.0f;
    g_game.new_record = false;
    g_game.state = Playing;
    SDL_ShowCursor(SDL_DISABLE);
    audio_play_gameplay_music();
}

void pause_play() {
    if (g_game.animation_progress == 0.0f) {
        update_play_state();
    }
    if (g_game.state == Playing) {
        audio_pause_music();
        g_game.state = Paused;
    }
}

void resume_play() {
    if (g_game.animation_progress == 0.0f) {
        g_game.frame++;
        g_game.animation_progress = (g_game.frame % CHAR_ANIM_FRAMES)/(double)CHAR_ANIM_FRAMES;
    }
    audio_resume_music();
    g_game.state = Playing;
}

void handle_playing_events(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        SDL_KeyCode sym = (SDL_KeyCode)event->key.keysym.sym;
        if (sym == g_game.key_pause) {
            if (event->key.repeat == 0) {
                pause_play();
            }
        }
        else if (g_game.ck_press == 0) {
            if (sym == g_game.key_left) { if (g_game.dhx == 0) { g_game.dhx = -1; g_game.dhy = 0; g_game.ck_press = 1; } }
            else if (sym == g_game.key_right) { if (g_game.dhx == 0) { g_game.dhx = 1; g_game.dhy = 0; g_game.ck_press = 1; } }
            else if (sym == g_game.key_up) { if (g_game.dhy == 0) { g_game.dhx = 0; g_game.dhy = -1; g_game.ck_press = 1; } }
            else if (sym == g_game.key_down) { if (g_game.dhy == 0) { g_game.dhx = 0; g_game.dhy = 1; g_game.ck_press = 1; } }
        }
    }
}

void handle_paused_events(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        if ((SDL_KeyCode)event->key.keysym.sym == g_game.key_pause) {
            if (event->key.repeat == 0) {
                resume_play();
            }
        }
    }
}
