#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include "text_renderer.h"
#include "pcg_basic.h"

#define WINDOW_TITLE "Vonsh" /* window title string */
#define WINDOW_W 900 /* windowed mode resolution */
#define WINDOW_H 700
#define VERSION_STR "v0.0" /* game version string */
#define RES_DIR "../share/games/vonsh/" /* resources directory */

typedef enum e_GameState {
    NotInitialized,
    NotStarted,
    Playing,
    Paused,
    GameOver
} GameState;

typedef enum e_FieldType { /* indicates what is inside game board field */
    Empty,
    Snake,
    Food,
    Wall
} FieldType;

typedef struct BoardField {
    FieldType type; /* type of this field */
    int p; /* paremeter(kind of character in snake body, kind of food, kind of wall ) */
    int ndx; /* delta x to next piece of snake. */
    int ndy; /* delta y to next piece of snake. */
    int pdx; /* delta x to previous piece of snake. */
    int pdy; /* delta y to previous piece of snake. */
} BoardField;

#define TILE_SIZE 16
#define GAME_SPEED 200
#define GROUND_TILES 8
#define WALL_TILES 4
#define FOOD_TILES 6
#define TOTAL_CHAR 24
#define CHAR_ANIM_FRAMES 4
#define FOOD_BLINK_T 27
SDL_Window *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *txt_game_board = NULL;
SDL_Surface *srf_env_tileset = NULL;
SDL_Texture *txt_env_tileset = NULL;
SDL_Surface *srf_food_tileset = NULL;
SDL_Texture *txt_food_tileset = NULL;
SDL_Surface *srf_food_marker = NULL;
SDL_Texture *txt_food_marker = NULL;
SDL_Surface *srf_char_tileset = NULL;
SDL_Texture *txt_char_tileset = NULL;
SDL_Surface *srf_font = NULL;
SDL_Texture *txt_font = NULL;
SDL_Surface *srf_logo = NULL;
SDL_Texture *txt_logo = NULL;
SDL_Surface *srf_trophy = NULL;
SDL_Texture *txt_trophy = NULL;
SDL_TimerID game_timer = 0;

Mix_Chunk *exp_chunk = NULL;
Mix_Chunk *die_chunk = NULL;
Mix_Music *idle_music = NULL;
Mix_Music *play_music = NULL;


SDL_Rect *ground_tile = NULL;
SDL_Rect *wall_tile = NULL;
SDL_Rect *food_tile = NULL;
BoardField *game_board = NULL;
GameState game_state = NotInitialized;

int game_board_w=0, game_board_h=0; /* game board width and height (in full tiles)*/
int screen_w=0, screen_h=0; /* game window width and height */
int fullscreen=0;     /* 1: full screen mode, 0: windowed mode */
int dhx=0, dhy=0;     /* head movement direction */
int hx=0, hy=0;       /* head coordinates */
int tx=0, ty=0;       /* tail coordinates */
int score=0;          /* player's score, number of food eaten */
int hi_score=0;       /* highest score from previous gameplays */
int new_record=0;     /* flag set if user beats previous record */
int expand_counter=0; /* counter of snake segments to add and walls to seed */
int ck_press=0;       /* control key pressed indicator */
int frame=0;          /* animation frame */
int music_on=1;       /* music enabled by default */
int sfx_on=1;         /* sound effects enabled by default */

/* Callback for animation frame timer.
   Generates SDL_USEREVENT when main game loop shall render next frame. */
uint32_t tick_callback(uint32_t interval, void *param) {
    SDL_Event event;
    SDL_UserEvent userevent;
    userevent.type = SDL_USEREVENT;  userevent.code = 0;
    userevent.data1 = NULL;  userevent.data2 = NULL;
    event.type = SDL_USEREVENT;  event.user = userevent;
    SDL_PushEvent(&event);
    return interval;
}

/********** GAME / PLAY INITIALIZATION FUNCTIONS ************/
/* Clears game board and fills the background texture with random grass pattern.
   Executed at start of the game and at start of play.
 */

void init_game_board(void) {
    /* Reset the game board array */
    for (int i=0; i<game_board_w*game_board_h; i++) {
        game_board[i].type = Empty;
        game_board[i].ndx = 0;
        game_board[i].ndy = 0;
        game_board[i].pdx = 0;
        game_board[i].pdy = 0;
    }

    /* Prepare uniform texture with background(grass + walls) - used for speeding up
       rendering of gameplay frames */
    /* Redirect rendering to the game board texture */
    SDL_SetRenderTarget(renderer, txt_game_board);
    /* Fill render target with black. Later status bar at the bottom of the screen
       will have that color. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    /* Fill background with random pattern of ground tiles */
    int x, y;
    SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    for (y=0; y<game_board_h; y++)
        for (x=0; x<game_board_w; x++) {
            DstR.x = x*TILE_SIZE; DstR.y = y*TILE_SIZE;
            SDL_RenderCopy(renderer, txt_env_tileset, &ground_tile[pcg32_boundedrand(GROUND_TILES)], &DstR);
        }

    /* Detach the game board texture from renderer */
    SDL_SetRenderTarget(renderer, NULL);
}

/*
    Load image from file and create texture using it
    retval 0 - texture loaded OK.
    retval 1 - texture not loaded
 */
static int load_texture(SDL_Surface **srf, SDL_Texture **txt, const char *filepath) {
    if ((*srf = IMG_Load(filepath)) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error loading image", filepath, NULL);
        return 1;
    }
    else if ((*txt = SDL_CreateTextureFromSurface(renderer, *srf)) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error creating texture", SDL_GetError(), NULL);
        return 1;
    }
    else {
        return 0;
    }
}

/*
    Initialize game engine
    retval 0 - initialization OK.
    retval 1 - error during init. Program has to be terminated
 */
int init_game(int sw, int sh, int fullscreen) {

    /* Executed only at start of program */
    if (game_state == NotInitialized) {
        /* init SDL library */
        if (SDL_Init(SDL_INIT_VIDEO)) {
            printf("Error initializing video subsystem: %s\n", SDL_GetError());
            return 1;
        }
        else if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO)) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "SDL initialization error", SDL_GetError(), NULL);
            return 1;
        }
        else if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "SDL Image initialization error", IMG_GetError(), NULL);
            return 1;
        }
        #define EXE_PATH_SIZE 200
        char exe_path[EXE_PATH_SIZE]; /* resources path */
        memset(exe_path, 0, EXE_PATH_SIZE);
        if (readlink("/proc/self/exe", exe_path, EXE_PATH_SIZE) == -1) {
            /* TODO: report precise cause of error */
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Work dir init error", "Error reading executable path", NULL);
            return 1;
        }
        if (chdir(dirname(exe_path)) == -1) {
            /* TODO: report precise cause of error */
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Work dir init error", "Error setting current working directory", NULL);
            return 1;
        }
        pcg32_srandom(time(NULL), 0x12345678); /* seed random number generator */
        if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096 ) == -1 ) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error opening audio device", Mix_GetError(), NULL);
            return 1;
        }
        Mix_AllocateChannels(4);

        if ((exp_chunk = Mix_LoadWAV(RES_DIR"exp_sound.wav")) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error loading sample", RES_DIR"exp_sound.wav", NULL);
            return 1;
        }
        if ((die_chunk = Mix_LoadWAV(RES_DIR"die_sound.wav")) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error loading sample", RES_DIR"die_sound.wav", NULL);
            return 1;
        }
        if ((idle_music = Mix_LoadMUS(RES_DIR"idle_tune.mp3")) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error loading music", RES_DIR"idle_tune.mp3", NULL);
            return 1;
        }
        if ((play_music = Mix_LoadMUS(RES_DIR"play_tune.mp3")) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error loading music", RES_DIR"play_tune.mp3", NULL);
            return 1;
        }
    }

    /* If fullscreen mode was selected, then set game window resolution to current desktop resolution and set
       game board dimensions accordingly */
    if (fullscreen) {
        if (game_state == NotInitialized) {
            if ((screen = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP)) == NULL) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    "SDL window not created", SDL_GetError(), NULL);
                return 1;
            }
        }
        else {
            SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        SDL_GetWindowSize(screen, &screen_w, &screen_h);

        game_board_w = screen_w/TILE_SIZE;
        game_board_h = screen_h/TILE_SIZE - 1;
    }
    /* If window mode was selected, then set game window to selected resolution and
       set game board dimensions accordingly */
    else {
        /* game board dimensions(in full tiles) */
        game_board_w = sw/TILE_SIZE;
        game_board_h = sh/TILE_SIZE - 1;
        /* game screen dimensions */
        screen_w = game_board_w*TILE_SIZE;
        /* bottom TILE_SIZE pixels is used for displaying score*/
        screen_h = (game_board_h+1)*TILE_SIZE;

        if (game_state == NotInitialized) {
            if ((screen = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, screen_w, screen_h, 0)) == NULL) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    "SDL window not created", SDL_GetError(), NULL);
                return 1;
            }
        }
        else {
            SDL_SetWindowFullscreen(screen, 0);
            SDL_SetWindowSize(screen, screen_w, screen_h);
            SDL_SetWindowPosition(screen, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    if (game_state == NotInitialized) {
        if ((renderer = SDL_CreateRenderer(screen, -1,
                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "SDL renderer not created", SDL_GetError(), NULL);
            return 1;
        }
    }
    if (game_state != NotInitialized) {
        /* when switching window resolution release previously created game board texture */
        SDL_DestroyTexture(txt_game_board);
        /* free previously allocated game board array */
        free(game_board);
    }
    if ((txt_game_board = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, screen_w, screen_h)) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error creating background texture", SDL_GetError(), NULL);
        return 1;
    }

    if ((game_board = calloc(game_board_w*game_board_h, sizeof(BoardField))) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error", "Error allocating memory", NULL);
        return 1;
    }

    /* Executed only at start of program */
    if (game_state == NotInitialized) {
        /* load textures */
        if (load_texture(&srf_env_tileset, &txt_env_tileset, RES_DIR"board_tiles.png") ||
            load_texture(&srf_food_tileset, &txt_food_tileset, RES_DIR"food_tiles.png") ||
            load_texture(&srf_food_marker, &txt_food_marker, RES_DIR"food_marker.png") ||
            load_texture(&srf_char_tileset, &txt_char_tileset, RES_DIR"character_tiles.png") ||
            load_texture(&srf_font, &txt_font, RES_DIR"good_neighbors.png") ||
            load_texture(&srf_logo, &txt_logo, RES_DIR"logo.png") ||
            load_texture(&srf_trophy, &txt_trophy, RES_DIR"trophy-bronze.png")) {
            /* loading of any of the textures failed */
            return 1;
        }

        /* set blending mode for food marker*/
        if (SDL_SetTextureBlendMode(txt_food_marker, SDL_BLENDMODE_ADD)) {
            return 1;
        }

        /* allocate tile info arrays */
        if ((ground_tile = calloc(GROUND_TILES, sizeof(SDL_Rect))) == NULL ||
            (wall_tile = calloc(WALL_TILES, sizeof(SDL_Rect))) == NULL ||
            (food_tile = calloc(FOOD_TILES, sizeof(SDL_Rect))) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error", "Error allocating memory", NULL);
            return 1;
        }
        /* fill tile info arrays */
        ground_tile[0].x = 0;   ground_tile[0].y = 0;
        ground_tile[1].x = 1;   ground_tile[1].y = 0;
        ground_tile[2].x = 2;   ground_tile[2].y = 0;
        ground_tile[3].x = 3;   ground_tile[3].y = 0;
        ground_tile[4].x = 4;   ground_tile[4].y = 0;
        ground_tile[5].x = 0;   ground_tile[5].y = 1;
        ground_tile[6].x = 0;   ground_tile[6].y = 2;
        ground_tile[7].x = 0;   ground_tile[7].y = 3;
        for (int i=0; i<GROUND_TILES; i++) {
            ground_tile[i].x *= TILE_SIZE;   ground_tile[i].y *= TILE_SIZE;
            ground_tile[i].w = ground_tile[i].h = TILE_SIZE;
        }
        wall_tile[0].x = 0;  wall_tile[0].y = 17;
        wall_tile[1].x = 1;  wall_tile[1].y = 17;
        wall_tile[2].x = 2;  wall_tile[2].y = 17;
        wall_tile[3].x = 3;  wall_tile[3].y = 17;
        for (int i=0; i<WALL_TILES; i++) {
            wall_tile[i].x *= TILE_SIZE;   wall_tile[i].y *= TILE_SIZE;
            wall_tile[i].w = wall_tile[i].h = TILE_SIZE;
        }
        food_tile[0].x = 4;  food_tile[0].y = 1;
        food_tile[1].x = 0;  food_tile[1].y = 4;
        food_tile[2].x = 1;  food_tile[2].y = 4;
        food_tile[3].x = 2;  food_tile[3].y = 4;
        food_tile[4].x = 0;  food_tile[4].y = 6;
        food_tile[5].x = 3;  food_tile[5].y = 6;
        for (int i=0; i<FOOD_TILES; i++) {
            food_tile[i].x *= TILE_SIZE;   food_tile[i].y *= TILE_SIZE;
            food_tile[i].w = food_tile[i].h = TILE_SIZE;
        }

        Mix_PlayMusic(idle_music, -1); /* TODO: might fail - shall we handle this? */
    }

    /* init text rendering functionality */
    init_text_renderer(renderer, txt_font);
    /* Reset game board and generate new background texture. */
    init_game_board();
    if (game_state == NotInitialized) {
        /* Start animation frame timer. */
        game_timer = SDL_AddTimer(GAME_SPEED/CHAR_ANIM_FRAMES, tick_callback, 0);
    }
    game_state = NotStarted;
    return 0;
}

void cleanup_game(void) {
    Mix_FreeChunk(exp_chunk);
    Mix_FreeChunk(die_chunk);
    Mix_CloseAudio();

    SDL_RemoveTimer(game_timer);

    SDL_DestroyTexture(txt_trophy);
    SDL_DestroyTexture(txt_logo);
    SDL_DestroyTexture(txt_font);
    SDL_DestroyTexture(txt_char_tileset);
    SDL_DestroyTexture(txt_env_tileset);
    SDL_DestroyTexture(txt_food_tileset);
    SDL_DestroyTexture(txt_food_marker);
    SDL_DestroyTexture(txt_game_board);

    SDL_FreeSurface(srf_trophy);
    SDL_FreeSurface(srf_logo);
    SDL_FreeSurface(srf_font);
    SDL_FreeSurface(srf_char_tileset);
    SDL_FreeSurface(srf_env_tileset);
    SDL_FreeSurface(srf_food_tileset);
    SDL_FreeSurface(srf_food_marker);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    IMG_Quit();
    SDL_Quit();

    free(food_tile);
    free(wall_tile);
    free(ground_tile);
    free(game_board);
}

/************ PLAY FUNCTIONS *************/
/* Adds obstacle or food at random location in game board and renders it to background texture. */
void seed_item(FieldType item_type)
{
    int board_offs = 0; /* offset of field in game board */
    int x = 0, y = 0; /* coordinates of field in game board */
    while(1) {
        board_offs = pcg32_boundedrand(game_board_w*game_board_h);
        if (game_board[board_offs].type == Empty) {
            game_board[board_offs].type = item_type;
            /* render seeded wall to game board texture */
            if (item_type == Wall) {
                x = board_offs % game_board_w;
                y = board_offs / game_board_w;
                SDL_Rect DstR = { x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE };
                /* Redirect rendering to the game board texture */
                SDL_SetRenderTarget(renderer, txt_game_board);
                /* Copy wall tile */
                SDL_RenderCopy(renderer, txt_env_tileset, &wall_tile[pcg32_boundedrand(WALL_TILES)], &DstR);
                /* Detach the game board texture from renderer */
                SDL_SetRenderTarget(renderer, NULL);
            }
            else if (item_type == Food) {
                game_board[board_offs].p = pcg32_boundedrand(FOOD_TILES);
            }
            break;
        }
    }
}

/* one iteration of game logic
    retval:
    0 - game continues
    1 - game over
 */
void update_play_state(void)
{
    /* verify all kinds of collisions */
    if (hx+dhx < 0             || hy+dhy < 0 ||
        hx+dhx >= game_board_w || hy+dhy >= game_board_h) {
        /* collision with game board edge */
        game_state = GameOver; /* GAME OVER */
    }
    else if (game_board[game_board_w*(hy+dhy)+hx+dhx].type == Food) {
        /* food hit - increase score, start expanding snake, seed new food */
        score++;
        if (score > hi_score) {
            hi_score = score;
            new_record = 1;
        }
        expand_counter += score;
        seed_item(Food);
    }
    else if (game_board[game_board_w*(hy+dhy)+hx+dhx].type != Empty) {
        /* collision with snake body or wall */
        game_state = GameOver; /* GAME OVER */
    }
    if (game_state == Playing) {
        int foff; /* game board field offset */
        /* move snake head */
        foff = game_board_w*hy+hx;
        game_board[foff].ndx = dhx; /* update "neck" direction towards new head position */
        game_board[foff].ndy = dhy;
        hx += dhx;    hy += dhy; /* update head position */
        foff = game_board_w*hy+hx;
        game_board[foff].type = Snake;
        game_board[foff].ndx = dhx; /* update head direction */
        game_board[foff].ndy = dhy;
        game_board[foff].pdx = -dhx;
        game_board[foff].pdy = -dhy;

        int ix = tx, iy = ty; /* indexes used for traversing snake body */
        int pb1, pb2; /* buffers for shifting character kinds from tail to head */
        /* shift character kinds from tail to head */
        pb1 = game_board[game_board_w*iy+ix].p;
        do {
            /* move character kind from previous position to next. */
            foff = game_board_w*iy+ix;
            ix = ix + game_board[foff].ndx;
            iy = iy + game_board[foff].ndy;
            foff = game_board_w*iy+ix;
            pb2 = game_board[foff].p; /* buffer current character kind... */
            game_board[foff].p = pb1; /* and overwrite it with previous kind... */
            pb1 = pb2; /* and then exchange buffers */
        } while(ix!=hx || iy!=hy);

        if (expand_counter==0) { /* move snake tail if not expanding */
            foff = game_board_w*ty+tx; /* tail offset on game board */
            tx = tx + game_board[foff].ndx;
            ty = ty + game_board[foff].ndy;
            game_board[foff].type = Empty;
            game_board[foff].p = 0;
            game_board[foff].ndx = game_board[foff].pdx = 0;
            game_board[foff].ndy = game_board[foff].pdy = 0;
        }
        else { /* expand tail and seed new obstacle */
            game_board[game_board_w*ty+tx].p = pcg32_boundedrand(TOTAL_CHAR);
            seed_item(Wall);
            expand_counter--;
            if (sfx_on) {
                Mix_VolumeChunk(exp_chunk, MIX_MAX_VOLUME/3);
                Mix_PlayChannel(-1, exp_chunk, 0); /* TODO: might fail - shall we handle this? */
            }
        }
        ck_press = 0; /* allow new control key press */
    }
    else if (game_state == GameOver) {
        if (sfx_on) {
            Mix_VolumeChunk(die_chunk, MIX_MAX_VOLUME/3);
            Mix_PlayChannel(-1, die_chunk, 0); /* TODO: might fail - shall we handle this? */
        }
        /* TODO: music shall start from beginning after starting game */
        Mix_PlayMusic(idle_music, -1); /* TODO: might fail - shall we handle this? */
        if (!music_on) {
            Mix_PauseMusic();
        }
    }
}

void start_play(void) {
    /* Reset game board and generate new background texture. */
    init_game_board();

    /* initialize movement direction and position */
    dhx = 0;   dhy = -1;
    score = expand_counter = 0;
    hx = tx = game_board_w/2;   hy = ty = game_board_h/2;
    /* seed first piece of snake */
    game_board[game_board_w*hy+hx].type = Snake;
    game_board[game_board_w*hy+hx].p = pcg32_boundedrand(TOTAL_CHAR);
    game_board[game_board_w*hy+hx].ndx = dhx;
    game_board[game_board_w*hy+hx].ndy = dhy;
    game_board[game_board_w*hy+hx].pdx = -dhx;
    game_board[game_board_w*hy+hx].pdy = -dhy;
    seed_item(Food);
    ck_press = 0;
    frame = 0;
    new_record = 0;
    game_state = Playing;
    /* TODO: 1. "play tune" shall start from beginning after new game is started
             2. After game over, for short moment when "die" sound is played,
                "play tune" is played, but it should be already "idle tune", or
                better no tune */
    Mix_PlayMusic(play_music, -1);
    if (!music_on) {
        Mix_PauseMusic();
    }
}

void pause_play() {
    game_state = Paused;
    if (music_on) {
        Mix_PauseMusic();
    }
}

void resume_play() {
    ck_press = 0;
    game_state = Playing;
    if (music_on) {
        Mix_ResumeMusic();
    }
}

/************ SCREEN RENDERING *************/
/* renders whole game state and blits everything to screen */
void render_screen(void)
{
    /* each frame everything is rendered from scratch */
    int x, y;
    int foff; /* game board field offset */
    SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    SDL_Rect SrcR = { 0, 0, TILE_SIZE, TILE_SIZE };
    char bottom_string[15];
    int char_frame = 0;
    int img_w, img_h;
    /* render game backround with walls */
    SDL_RenderCopy(renderer, txt_game_board, NULL, NULL);

    if (game_state != GameOver) {
        char_frame = frame % CHAR_ANIM_FRAMES; /* frame in character animation */
    }
    else {
        char_frame = 0;
    }
    /* render snake body and food*/
    foff = 0; /* offset to game board field */
    for (y=0; y<game_board_h; y++) {
        for (x=0; x<game_board_w; x++) {
            switch(game_board[foff].type) {
                case Empty:
                case Wall:
                    /* empty fields and obstacles were already copied from txt_game_board */
                    break;
                case Snake:
                    /* select direction of character based on snake piece direction */
                    if (-game_board[foff].pdx == 0 && -game_board[foff].pdy == 1) {
                        SrcR.x = 0;
                    }
                    else if (-game_board[foff].pdx == 1 && -game_board[foff].pdy == 0) {
                        SrcR.x = TILE_SIZE;
                    }
                    else if (-game_board[foff].pdx == 0 && -game_board[foff].pdy == -1) {
                        SrcR.x = 2*TILE_SIZE;
                    }
                    else if (-game_board[foff].pdx == -1 && -game_board[foff].pdy == 0) {
                        SrcR.x = 3*TILE_SIZE;
                    }
                    /* select type of character based on game board field parameter */
                    SrcR.y = game_board[foff].p*3*(TILE_SIZE+1) + 1;
                    /* animate character - select animation frame */
                    if (char_frame == 1) {
                        SrcR.y += (TILE_SIZE+1);
                    }
                    else if (char_frame == 3) {
                        SrcR.y += 2*(TILE_SIZE+1);
                    }
                    /* select character position on screen */
                    DstR.x = x*TILE_SIZE;
                    DstR.y = y*TILE_SIZE;
                    /* animate position of characters between board fields */
                    if (game_state != GameOver) {
                        DstR.x += (CHAR_ANIM_FRAMES-char_frame) * game_board[foff].pdx*\
                                  TILE_SIZE/CHAR_ANIM_FRAMES;
                        DstR.y += (CHAR_ANIM_FRAMES-char_frame) * game_board[foff].pdy*\
                                  TILE_SIZE/CHAR_ANIM_FRAMES;
                    }
                    /* render character to screen buffer */
                    SDL_RenderCopy(renderer, txt_char_tileset, &SrcR, &DstR);
                    break;
                case Food:
                    DstR.x = x*TILE_SIZE;
                    DstR.y = y*TILE_SIZE;
                    /* blink food item during gameplay */
                    if (game_state == Playing && frame % (FOOD_BLINK_T*3) < FOOD_BLINK_T) {
                        if ((frame/3) % 3 == 0) {
                            SDL_RenderCopy(renderer, txt_food_marker, NULL, &DstR);
                        }
                        else if ((frame/3) % 3 == 1) {
                            SDL_RenderCopy(renderer, txt_food_tileset,
                                           &food_tile[game_board[foff].p], &DstR);
                        }
                        else {
                            /* don't render anything */
                        };
                    }
                    /* don't blink, just render the food */
                    else {
                        SDL_RenderCopy(renderer, txt_food_tileset,
                                       &food_tile[game_board[foff].p], &DstR);
                    }
                    break;
                default:
                    break;
            }
            foff++; /* next game board field */
        }
    }

    int yc;
    /* Extra elements dependent on current game state. */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    switch (game_state) {
        case NotStarted:
            /* "First start" screen */
            SDL_QueryTexture(txt_logo, NULL, NULL, &img_w, &img_h);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 127);
            DstR.w = 16*TILE_SIZE;        DstR.h = (2+8.5)*TILE_SIZE+img_h;
            yc = (screen_h-TILE_SIZE-DstR.h)/2;
            DstR.x = (screen_w-DstR.w)/2; DstR.y = yc;
            SDL_RenderFillRect(renderer, &DstR);
            yc += TILE_SIZE;
            DstR.w = img_w;              DstR.h = img_h;
            DstR.x = (screen_w-DstR.w)/2; DstR.y = yc;
            SDL_RenderCopy(renderer, txt_logo, NULL, &DstR);
            yc += img_h;
            SET_GREY_TEXT;
            render_text(screen_w/2,    yc,             Center, VERSION_STR);
            yc += TILE_SIZE;
            SET_YELLOW_TEXT;
            render_text(screen_w/2-32, yc,             Right, "Arrows:");
            render_text(screen_w/2-32, yc+TILE_SIZE,   Right, "SPACE:");
            render_text(screen_w/2-32, yc+2*TILE_SIZE, Right, "Z:");
            render_text(screen_w/2-32, yc+3*TILE_SIZE, Right, "X:");
            render_text(screen_w/2-32, yc+4*TILE_SIZE, Right, "S:");
            render_text(screen_w/2-32, yc+5*TILE_SIZE, Right, "ESC:");
            SET_GREY_TEXT;
            render_text(screen_w/2-32, yc,             Left, " Snake control");
            render_text(screen_w/2-32, yc+TILE_SIZE,   Left, " Pause/Resume");
            render_text(screen_w/2-32, yc+2*TILE_SIZE, Left, " Music");
            render_text(screen_w/2-32, yc+3*TILE_SIZE, Left, " Sound effects");
            render_text(screen_w/2-32, yc+4*TILE_SIZE, Left, " Window/Full");
            render_text(screen_w/2-32, yc+5*TILE_SIZE, Left, " Quit");
            SET_WHITE_TEXT;
            render_text(screen_w/2,    yc+6.5*TILE_SIZE, Center, "Press SPACE to play");
            break;
        case GameOver:
            /* "Game Over" screen */
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 127);
            SDL_QueryTexture(txt_trophy, NULL, NULL, &img_w, &img_h);
            DstR.w = 16*TILE_SIZE;
            DstR.h = new_record ? 6*TILE_SIZE+img_h : 4.5*TILE_SIZE;
            yc = (screen_h-TILE_SIZE-DstR.h)/2;
            DstR.x = (screen_w-DstR.w)/2; DstR.y = yc;
            SDL_RenderFillRect(renderer, &DstR);
            yc += TILE_SIZE;
            SET_WHITE_TEXT;
            render_text(screen_w/2, yc, Center, "Game Over");
            yc += 1.5*TILE_SIZE;
            if (new_record) {
                DstR.w = img_w;               DstR.h = img_h;
                DstR.x = (screen_w-DstR.w)/2; DstR.y = yc;
                SDL_RenderCopy(renderer, txt_trophy, NULL, &DstR);
                yc += DstR.h;
                if (frame%15<10) {
                    SET_YELLOW_TEXT;
                    render_text(screen_w/2, yc, Center, "NEW HI SCORE !");
                }
                yc += 1.5*TILE_SIZE;
            }
            SET_GREY_TEXT;
            render_text(screen_w/2, yc, Center, "Press SPACE to play again");
            break;
        case Playing:
            break;
        case Paused:
            SET_WHITE_TEXT;
            render_text(screen_w/2, screen_h/2-TILE_SIZE, Center, "Pause");
            break;
        default:
            break;
    }
    /* print score, hi score and music/sound effects state*/
    SET_WHITE_TEXT;
    sprintf(bottom_string, "HI SCORE: %d", hi_score);
    render_text(TILE_SIZE, game_board_h*TILE_SIZE, Left, bottom_string);
    sprintf(bottom_string, "SCORE: %d", score);
    render_text(screen_w-TILE_SIZE, game_board_h*TILE_SIZE, Right, bottom_string);
    SET_GREY_TEXT;
    if (music_on) {
        render_text(screen_w/2, game_board_h*TILE_SIZE, Right, "Music ");
    }
    if (sfx_on) {
        render_text(screen_w/2, game_board_h*TILE_SIZE, Left, "  SFX");
    }
    SDL_RenderPresent(renderer);
}


/*############ MAIN GAME LOOP #############*/
int main(int argc, char ** argv)
{
    int quit = 0;
    SDL_Event event;

    /* basic game configuration */
    if (init_game(WINDOW_W, WINDOW_H, fullscreen)) {
        /* Error during init. Terminating game. */
        quit = 1;
    }
    else {
        render_screen();
    }

    while (!quit)
    {
        SDL_WaitEvent(&event);
        switch (event.type)
        {
            case SDL_USEREVENT: /* game tick event */
                if (game_state == Paused) {
                    /* animations are played in every state except "Paused" */
                    break;
                }
                frame++;
                if (game_state == Playing) {
                    /* play state update and animations if playing */
                    if (frame % CHAR_ANIM_FRAMES == 0) { /* updated play state frame */
                        /* update play state */
                        update_play_state();
                    }
                    /* render each frame - both animations and "full" updated play state frames */
                    render_screen();
                }
                else if (game_state == GameOver) {
                    render_screen();
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                else if (event.key.keysym.sym == SDLK_SPACE) {
                    switch (game_state) {
                        case NotStarted:
                        case GameOver:
                            /* start new play */
                            start_play();
                            render_screen();
                            break;
                        case Playing:
                            /* pause play */
                            pause_play();
                            render_screen();
                            break;
                        case Paused:
                            /* resume play */
                            resume_play();
                            break;
                        default:
                            break;
                    }
                }
                else if (event.key.keysym.sym == SDLK_z && event.key.repeat == 0) {
                    /* toggle music on/off */
                    music_on = !music_on;
                    render_screen();
                    if (game_state != Paused) {
                        if (music_on) {
                            Mix_ResumeMusic();
                        }
                        else {
                            Mix_PauseMusic();
                        }
                    }
                }
                else if (event.key.keysym.sym == SDLK_x && event.key.repeat == 0) {
                    /* toggle sound effects on/off */
                    sfx_on = !sfx_on;
                    render_screen();
                }
                else if (event.key.keysym.sym == SDLK_s && event.key.repeat == 0 &&
                        (game_state == NotStarted || game_state == NotInitialized)) {
                    /* toggle display between windowed/fullscreen modes */
                    fullscreen = !fullscreen;
                    if (init_game(WINDOW_W, WINDOW_H, fullscreen)) {
                        quit = 1;
                    }
                    else {
                        SDL_Delay(50);
                        render_screen();
                    }
                }
                else if (game_state == Playing && event.key.repeat == 0 && ck_press == 0) {
                    switch (event.key.keysym.sym) {
                        case SDLK_LEFT:  if (dhx == 0) { dhx = -1; dhy = 0; ck_press = 1; } break;
                        case SDLK_RIGHT: if (dhx == 0) { dhx = 1; dhy = 0; ck_press = 1; } break;
                        case SDLK_UP:    if (dhy == 0) { dhx = 0; dhy = -1; ck_press = 1; } break;
                        case SDLK_DOWN:  if (dhy == 0) { dhx = 0; dhy = 1; ck_press = 1; } break;
                    }
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
        }
    }
    cleanup_game();
    return 0;
}
