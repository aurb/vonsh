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
#define BOARD_WIDTH (56)
#define BOARD_HEIGHT (41)
#define RES_DIR "../share/games/vonsh/" /* resources directory */
#define TILE_SIZE (16)
#define MENU_WIDTH (20*16)
#define MENU_BORDER (20)
#define MENU_LOGO_SPACE (15)
#define MENU_HEAD_SPACE (24)
#define MENU_ITEM_WIDTH (180)
#define MENU_ITEM_HEIGHT (24)
#define GAME_OVER_WIDTH (16*16)
#define GAME_OVER_BORDER (16)
#define GAME_OVER_ITEM_SPACE (12)
typedef enum e_GameState {
    NotInitialized,
    NotStarted,
    Playing,
    Paused,
    GameOver,
    TitleScreen,
    OptionsMenu,
    KeyConfiguring,
    EditingBoardWidth,   // NEW: editing board width
    EditingBoardHeight   // NEW: editing board height
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

#define RENDER_INTERVAL 50   // Interval between frames in milliseconds
#define GROUND_TILES 8  // Number of ground tiles in the tileset
#define WALL_TILES 4  // Number of wall tiles in the tileset
#define FOOD_TILES 6  // Number of food tiles in the tileset
#define TOTAL_CHAR 24  // Total number of characters in the tileset
#define CHAR_ANIM_FRAMES 4  // Number of animation frames for characters
#define FOOD_BLINK_FRAMES 27  // Number of frames in blink food animation
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

int win_board_w=BOARD_WIDTH, win_board_h=BOARD_HEIGHT; /* windowed mode game board width and height (in full tiles)*/
int board_w=0, board_h=0; /* current game board width and height (in full tiles)*/
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
float animation_progress = 0.0f; /* Animation progress between frames */
int music_on=1;       /* music enabled by default */
int sfx_on=1;         /* sound effects enabled by default */

/* Key configuration */
SDL_KeyCode key_left = SDLK_LEFT;
SDL_KeyCode key_right = SDLK_RIGHT;
SDL_KeyCode key_up = SDLK_UP;
SDL_KeyCode key_down = SDLK_DOWN;
SDL_KeyCode key_pause = SDLK_SPACE;  // Fixed to Space key

typedef struct MenuItem {
    const char* text;
    void (*action)(void);
    SDL_Rect rect; // For mouse collision
    int is_key_config; // Flag to indicate if this is a key configuration item
    const char* config_label; // e.g., "Left:"
    SDL_KeyCode* key_to_configure; // Pointer to the actual key variable
} MenuItem;

// Menu actions
void menu_action_play(void);
void menu_action_options(void);
void menu_action_exit(void);
void menu_action_back_to_title(void);
void menu_action_toggle_music(void);
void menu_action_toggle_sfx(void);
void menu_action_toggle_fullscreen(void);
void menu_action_configure_key(MenuItem* item); // Placeholder, will pass specific item

// Title Screen Menu
MenuItem title_menu_items[] = {
    {"Play", menu_action_play, {0,0,0,0}, 0, NULL, NULL},
    {"Options", menu_action_options, {0,0,0,0}, 0, NULL, NULL},
    {"Exit", menu_action_exit, {0,0,0,0}, 0, NULL, NULL}
};
int title_menu_item_count = sizeof(title_menu_items) / sizeof(MenuItem);
int title_menu_selected_item = 0;

// Indices for new menu items (after key config, before pause/music/sfx)
#define OPTIONS_IDX_KEY_LEFT   0
#define OPTIONS_IDX_KEY_RIGHT  1
#define OPTIONS_IDX_KEY_UP     2
#define OPTIONS_IDX_KEY_DOWN   3
#define OPTIONS_IDX_WIDTH      4
#define OPTIONS_IDX_HEIGHT     5
#define OPTIONS_IDX_PAUSE      6
#define OPTIONS_IDX_MUSIC      7
#define OPTIONS_IDX_SFX        8
#define OPTIONS_IDX_FULLSCREEN 9
#define OPTIONS_IDX_BACK      10

// Minimum allowed board size
#define MIN_BOARD_WIDTH 30
#define MIN_BOARD_HEIGHT 20

// Input buffer for editing width/height
char board_dim_input[8] = {0};
int editing_field = -1; // 0: width, 1: height

// Options Screen Menu (redefine with new entries)
MenuItem options_menu_items[] = {
    {NULL, NULL, {0,0,0,0}, 1, "Left:", &key_left},
    {NULL, NULL, {0,0,0,0}, 1, "Right:", &key_right},
    {NULL, NULL, {0,0,0,0}, 1, "Up:", &key_up},
    {NULL, NULL, {0,0,0,0}, 1, "Down:", &key_down},
    {NULL, NULL, {0,0,0,0}, 0, NULL, NULL}, // Board width
    {NULL, NULL, {0,0,0,0}, 0, NULL, NULL}, // Board height
    {"Pause: SPACE", NULL, {0,0,0,0}, 0, NULL, NULL},
    {NULL, menu_action_toggle_music, {0,0,0,0}, 0, NULL, NULL},
    {NULL, menu_action_toggle_sfx, {0,0,0,0}, 0, NULL, NULL},
    {NULL, menu_action_toggle_fullscreen, {0,0,0,0}, 0, NULL, NULL},
    {"Back", menu_action_back_to_title, {0,0,0,0}, 0, NULL, NULL}
};
int options_menu_item_count = sizeof(options_menu_items) / sizeof(MenuItem);
int options_menu_selected_item = 0;
int configuring_key_item_index = -1; // Index of the item being configured

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
    for (int i=0; i<board_w*board_h; i++) {
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
    for (y=0; y<board_h; y++)
        for (x=0; x<board_w; x++) {
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

/* Creates display area in windowed mode based on current game board dimensions */
static int create_windowed_display(void) {
    board_w = win_board_w;
    board_h = win_board_h;
    screen_w = board_w*TILE_SIZE;
    screen_h = (board_h+1)*TILE_SIZE;

    if (game_state == NotInitialized) {
        if ((screen = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                     SDL_WINDOWPOS_CENTERED, screen_w, screen_h, 0)) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "SDL window not created", SDL_GetError(), NULL);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error", "Failed to switch to windowed mode", NULL);
            return 1;
        }
    } else {
        SDL_SetWindowFullscreen(screen, 0);
        SDL_SetWindowSize(screen, screen_w, screen_h);
        SDL_SetWindowPosition(screen, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    
    return 0;
}

/* Creates display area in fullscreen mode and updates game board dimensions */
static int create_fullscreen_display(void) {
    if (game_state == NotInitialized) {
        if ((screen = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                     SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP)) == NULL) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "SDL window not created", SDL_GetError(), NULL);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Error", "Failed to switch to fullscreen mode", NULL);
            return 1;
        }
    } else {
        SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    
    SDL_GetWindowSize(screen, &screen_w, &screen_h);
    board_w = screen_w/TILE_SIZE;
    board_h = screen_h/TILE_SIZE - 1;
    return 0;
}

/*
    Initialize game engine
    retval 0 - initialization OK.
    retval 1 - error during init. Program has to be terminated
 */
int init_game_engine(int sw, int sh, int fullscreen) {
    /* Executed only at start of program */
    if (game_state != NotInitialized) {
        printf("Error: init_game_engin called in wrong state\n");
        return 1;
    }

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
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Work dir init error", "Error reading executable path", NULL);
        return 1;
    }
    if (chdir(dirname(exe_path)) == -1) {
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

    /* Create display area based on mode */
    if (fullscreen) {
        if (create_fullscreen_display()) return 1;
    } else {
        if (create_windowed_display()) return 1;
    }

    if ((renderer = SDL_CreateRenderer(screen, -1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "SDL renderer not created", SDL_GetError(), NULL);
        return 1;
    }
    if ((txt_game_board = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, screen_w, screen_h)) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error creating background texture", SDL_GetError(), NULL);
        return 1;
    }

    if ((game_board = calloc(board_w*board_h, sizeof(BoardField))) == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error", "Error allocating memory", NULL);
        return 1;
    }

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

    if (Mix_PlayMusic(idle_music, -1) == -1) {} /* TODO: might fail - shall we handle this? */

    /* init text rendering functionality */
    init_text_renderer(renderer, txt_font);
    /* Reset game board and generate new background texture. */
    init_game_board();
    game_state = NotStarted;
    return 0;
}

void cleanup_game(void) {
    Mix_HaltMusic();     /* stop music playback */
    Mix_HaltChannel(-1); /* stop all audio channels playback */
    Mix_FreeMusic(idle_music);
    Mix_FreeMusic(play_music);
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
        board_offs = pcg32_boundedrand(board_w*board_h);
        if (game_board[board_offs].type == Empty) {
            game_board[board_offs].type = item_type;
            /* render seeded wall to game board texture */
            if (item_type == Wall) {
                x = board_offs % board_w;
                y = board_offs / board_w;
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
        hx+dhx >= board_w || hy+dhy >= board_h) {
        /* collision with game board edge */
        game_state = GameOver; /* GAME OVER */
    }
    else if (game_board[board_w*(hy+dhy)+hx+dhx].type == Food) {
        /* food hit - increase score, start expanding snake, seed new food */
        score++;
        if (score > hi_score) {
            hi_score = score;
            new_record = 1;
        }
        expand_counter += score;
        seed_item(Food);
    }
    else if (game_board[board_w*(hy+dhy)+hx+dhx].type != Empty) {
        /* collision with snake body or wall */
        game_state = GameOver; /* GAME OVER */
    }
    if (game_state == Playing) {
        int foff; /* game board field offset */
        /* move snake head */
        foff = board_w*hy+hx;
        game_board[foff].ndx = dhx; /* update "neck" direction towards new head position */
        game_board[foff].ndy = dhy;
        hx += dhx;    hy += dhy; /* update head position */
        foff = board_w*hy+hx;
        game_board[foff].type = Snake;
        game_board[foff].ndx = dhx; /* update head direction */
        game_board[foff].ndy = dhy;
        game_board[foff].pdx = -dhx;
        game_board[foff].pdy = -dhy;

        int ix = tx, iy = ty; /* indexes used for traversing snake body */
        int pb1, pb2; /* buffers for shifting character kinds from tail to head */
        /* shift character kinds from tail to head */
        pb1 = game_board[board_w*iy+ix].p;
        do {
            /* move character kind from previous position to next. */
            foff = board_w*iy+ix;
            ix = ix + game_board[foff].ndx;
            iy = iy + game_board[foff].ndy;
            foff = board_w*iy+ix;
            pb2 = game_board[foff].p; /* buffer current character kind... */
            game_board[foff].p = pb1; /* and overwrite it with previous kind... */
            pb1 = pb2; /* and then exchange buffers */
        } while(ix!=hx || iy!=hy);

        if (expand_counter==0) { /* move snake tail if not expanding */
            foff = board_w*ty+tx; /* tail offset on game board */
            tx = tx + game_board[foff].ndx;
            ty = ty + game_board[foff].ndy;
            game_board[foff].type = Empty;
            game_board[foff].p = 0;
            game_board[foff].ndx = game_board[foff].pdx = 0;
            game_board[foff].ndy = game_board[foff].pdy = 0;
        }
        else { /* expand tail and seed new obstacle */
            game_board[board_w*ty+tx].p = pcg32_boundedrand(TOTAL_CHAR);
            seed_item(Wall);
            expand_counter--;
            if (sfx_on) {
                Mix_VolumeChunk(exp_chunk, MIX_MAX_VOLUME/3);
                if (Mix_PlayChannel(-1, exp_chunk, 0) == -1) {} /* TODO: might fail - shall we handle this? */
            }
        }
        ck_press = 0; /* allow new control key press */
    }
    else if (game_state == GameOver) {
        if (sfx_on) {
            Mix_VolumeChunk(die_chunk, MIX_MAX_VOLUME/3);
            if (Mix_PlayChannel(-1, die_chunk, 0) == -1) {} /* TODO: might fail - shall we handle this? */
        }
        if (Mix_PlayMusic(idle_music, -1) == -1) {} /* TODO: might fail - shall we handle this? */
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
    hx = tx = board_w/2;   hy = ty = board_h/2;
    /* seed first piece of snake */
    game_board[board_w*hy+hx].type = Snake;
    game_board[board_w*hy+hx].p = pcg32_boundedrand(TOTAL_CHAR);
    game_board[board_w*hy+hx].ndx = dhx;
    game_board[board_w*hy+hx].ndy = dhy;
    game_board[board_w*hy+hx].pdx = -dhx;
    game_board[board_w*hy+hx].pdy = -dhy;
    seed_item(Food);
    ck_press = 0;
    frame = 0;
    animation_progress = 0.0f;
    new_record = 0;
    game_state = Playing;
    if (Mix_PlayMusic(play_music, -1) == -1) {} /* TODO: might fail - shall we handle this? */
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
    int x,y,l;
    char txt_buf[40]; /* buffer for rendering text strings */

    /* clear screen with black */
    SDL_SetRenderDrawColor(renderer, 0,0,0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    if (game_state == TitleScreen || game_state == OptionsMenu || game_state == KeyConfiguring) {
        // Common background for Title and Options
        // Fill with pattern
        SDL_Rect DstR_bg = { 0, 0, TILE_SIZE, TILE_SIZE };
        for (y=0; y < screen_h / TILE_SIZE +1; y++) {
            for (x=0; x < screen_w / TILE_SIZE +1; x++) {
                DstR_bg.x = x*TILE_SIZE; DstR_bg.y = y*TILE_SIZE;
                SDL_RenderCopy(renderer, txt_env_tileset, &ground_tile[pcg32_boundedrand(GROUND_TILES)], &DstR_bg);
            }
        }
        // Dark rectangle
        int text_height = get_text_height("X");
        SDL_Rect dark_rect;
        dark_rect.w = MENU_WIDTH;
        dark_rect.x = (screen_w - dark_rect.w) / 2;
        SDL_Rect logo_dst;
        SDL_QueryTexture(txt_logo, NULL, NULL, &logo_dst.w, &logo_dst.h);
        
        if (game_state == TitleScreen) {
            dark_rect.h = 2*MENU_BORDER + text_height + MENU_LOGO_SPACE + logo_dst.h + MENU_HEAD_SPACE + 
                         ((title_menu_item_count-1) * MENU_ITEM_HEIGHT);
            dark_rect.y = (screen_h - dark_rect.h) / 2;
        } else if (game_state == OptionsMenu || game_state == KeyConfiguring) {
            dark_rect.h = 2*MENU_BORDER + text_height + MENU_LOGO_SPACE + logo_dst.h + MENU_HEAD_SPACE + 
                         ((options_menu_item_count-1) * MENU_ITEM_HEIGHT);
            dark_rect.y = (screen_h - dark_rect.h) / 2;
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &dark_rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Logo
        logo_dst.x = screen_w/2 - logo_dst.w/2;
        logo_dst.y = dark_rect.y + MENU_BORDER;
        SDL_RenderCopy(renderer, txt_logo, NULL, &logo_dst);

        int current_y = logo_dst.y + logo_dst.h + MENU_LOGO_SPACE;

        if (game_state == TitleScreen) {
            // Version string
            sprintf(txt_buf, "version: %s", VERSION_STR);
            SET_GREY_TEXT;
            render_text(renderer, txt_font, txt_buf, screen_w/2, current_y, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
            current_y += MENU_HEAD_SPACE;

            // Draw Title Menu
            for (int i = 0; i < title_menu_item_count; i++) {
                if (i == 0) {
                    SET_YELLOW_TEXT;
                } else {
                    SET_WHITE_TEXT;
                }
                title_menu_items[i].rect.w = MENU_ITEM_WIDTH;
                title_menu_items[i].rect.h = MENU_ITEM_HEIGHT;
                title_menu_items[i].rect.x = screen_w/2 - MENU_ITEM_WIDTH/2;
                title_menu_items[i].rect.y = current_y;
                render_text(renderer, txt_font, title_menu_items[i].text, screen_w/2, current_y + MENU_ITEM_HEIGHT/2, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
                if (i == title_menu_selected_item) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                    SDL_RenderDrawRect(renderer, &title_menu_items[i].rect);
                }
                current_y += MENU_ITEM_HEIGHT;
            }
        } else if (game_state == OptionsMenu || game_state == KeyConfiguring) {
            // "Options" title
            SET_GREY_TEXT;
            render_text(renderer, txt_font, "Options", screen_w/2, current_y, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
            current_y += MENU_HEAD_SPACE;

            // Draw Options Menu
            SET_WHITE_TEXT;
            for (int i = 0; i < options_menu_item_count; i++) {
                char item_text_buf[100];
                SET_WHITE_TEXT;
                if (options_menu_items[i].is_key_config) {
                    const char* key_name = SDL_GetKeyName(*(options_menu_items[i].key_to_configure));
                    if (game_state == KeyConfiguring && configuring_key_item_index == i) {
                         sprintf(item_text_buf, "%s ...", options_menu_items[i].config_label);
                    }
                     else {
                         sprintf(item_text_buf, "%s %s", options_menu_items[i].config_label, key_name);
                    }
                } else if (i == OPTIONS_IDX_WIDTH) {
                    // Board width entry
                    if (fullscreen) {
                        SET_GREY_TEXT;
                        sprintf(item_text_buf, "Board width: %d", board_w);
                    } else if (game_state == EditingBoardWidth) {
                        SET_YELLOW_TEXT;
                        sprintf(item_text_buf, "Board width: %s_", board_dim_input);
                    } else {
                        sprintf(item_text_buf, "Board width: %d", win_board_w);
                    }
                } else if (i == OPTIONS_IDX_HEIGHT) {
                    // Board height entry
                    if (fullscreen) {
                        SET_GREY_TEXT;
                        sprintf(item_text_buf, "Board height: %d", board_h);
                    } else if (game_state == EditingBoardHeight) {
                        SET_YELLOW_TEXT;
                        sprintf(item_text_buf, "Board height: %s_", board_dim_input);
                    } else {
                        sprintf(item_text_buf, "Board height: %d", win_board_h);
                    }
                } else if (options_menu_items[i].action == menu_action_toggle_music) {
                    sprintf(item_text_buf, "Music: %s", music_on ? "ON" : "OFF");
                } else if (options_menu_items[i].action == menu_action_toggle_sfx) {
                    sprintf(item_text_buf, "Sound effects: %s", sfx_on ? "ON" : "OFF");
                } else if (options_menu_items[i].action == menu_action_toggle_fullscreen) {
                    sprintf(item_text_buf, "Display: %s", fullscreen ? "fullscreen" : "window");
                } else {
                    if (options_menu_items[i].action == NULL) {
                        SET_GREY_TEXT;
                    }
                    sprintf(item_text_buf, "%s", options_menu_items[i].text);
                }

                options_menu_items[i].rect.w = MENU_ITEM_WIDTH;
                options_menu_items[i].rect.h = MENU_ITEM_HEIGHT;
                options_menu_items[i].rect.x = screen_w/2 - MENU_ITEM_WIDTH/2;
                options_menu_items[i].rect.y = current_y;
                if (i == options_menu_item_count-1) {
                    SET_YELLOW_TEXT;
                }
                render_text(renderer, txt_font, item_text_buf, screen_w/2, current_y + MENU_ITEM_HEIGHT/2, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);

                if (i == options_menu_selected_item && game_state != EditingBoardWidth && game_state != EditingBoardHeight) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                    SDL_RenderDrawRect(renderer, &options_menu_items[i].rect);
                }
                current_y += MENU_ITEM_HEIGHT;
            }
        }
    }
    else if (game_state == Playing || game_state == Paused || game_state == GameOver) {
        SDL_RenderCopy(renderer, txt_game_board, NULL, NULL);

        int x, y;
        int foff; /* game board field offset */
        SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
        SDL_Rect SrcR = { 0, 0, TILE_SIZE, TILE_SIZE };
        /* render snake body and food*/
        foff = 0; /* offset to game board field */
        for (y=0; y<board_h; y++) {
            for (x=0; x<board_w; x++) {
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
                        int anim_frame = (int)(animation_progress * CHAR_ANIM_FRAMES);
                        if (anim_frame == 1) {
                            SrcR.y += (TILE_SIZE+1);
                        }
                        else if (anim_frame == 3) {
                            SrcR.y += 2*(TILE_SIZE+1);
                        }
                        /* select character position on screen */
                        DstR.x = x*TILE_SIZE;
                        DstR.y = y*TILE_SIZE;
                        /* animate position of characters between board fields */
                        if (game_state != GameOver) {
                            DstR.x += (1.0f - animation_progress) * game_board[foff].pdx * TILE_SIZE;
                            DstR.y += (1.0f - animation_progress) * game_board[foff].pdy * TILE_SIZE;
                        }
                        /* render character to screen buffer */
                        SDL_RenderCopy(renderer, txt_char_tileset, &SrcR, &DstR);
                        break;
                    case Food:
                        DstR.x = x*TILE_SIZE;
                        DstR.y = y*TILE_SIZE;
                        /* blink food item during gameplay */
                        if (game_state == Playing && frame % (FOOD_BLINK_FRAMES*3) < FOOD_BLINK_FRAMES) {
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
            case GameOver:
                /* "Game Over" screen */
                int text_height = get_text_height("X");
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 127);
                SDL_QueryTexture(txt_trophy, NULL, NULL, &l, &l);
                DstR.w = GAME_OVER_WIDTH;
                DstR.h = 2*GAME_OVER_BORDER + 2*text_height + GAME_OVER_ITEM_SPACE;
                if (new_record) {
                    DstR.h += l + text_height +2*GAME_OVER_ITEM_SPACE;
                }
                yc = (screen_h-TILE_SIZE-DstR.h)/2;
                DstR.x = (screen_w-DstR.w)/2; DstR.y = yc;
                SDL_RenderFillRect(renderer, &DstR);
                yc += GAME_OVER_BORDER + text_height/2;
                SET_WHITE_TEXT;
                render_text(renderer, txt_font, "Game Over", screen_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
                if (new_record) {
                    yc += text_height/2 + GAME_OVER_ITEM_SPACE;
                    DstR.w = l;               DstR.h = l;
                    DstR.x = (screen_w-DstR.w)/2; DstR.y = yc;
                    SDL_RenderCopy(renderer, txt_trophy, NULL, &DstR);
                    yc += DstR.h + text_height/2 + GAME_OVER_ITEM_SPACE;
                    if (frame%15<10) {
                        SET_YELLOW_TEXT;
                        render_text(renderer, txt_font, "NEW HI SCORE !", screen_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
                    }
                }
                yc += text_height + GAME_OVER_ITEM_SPACE;
                SET_GREY_TEXT;
                render_text(renderer, txt_font, "Press SPACE", screen_w/2, yc, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
                break;
            case Paused:
                SET_WHITE_TEXT;
                render_text(renderer, txt_font, "Pause", screen_w/2, screen_h/2-TILE_SIZE, ALIGN_CENTER_HORIZONTAL, ALIGN_CENTER_VERTICAL);
                break;
            default:
                break;
        }
        /* print score, hi score and music/sound effects state*/
        SET_WHITE_TEXT;
        sprintf(txt_buf, "HI SCORE: %d", hi_score);
        render_text(renderer, txt_font, txt_buf, TILE_SIZE, board_h*TILE_SIZE, ALIGN_LEFT, ALIGN_TOP);
        sprintf(txt_buf, "SCORE: %d", score);
        render_text(renderer, txt_font, txt_buf, screen_w-TILE_SIZE, board_h*TILE_SIZE, ALIGN_RIGHT, ALIGN_TOP);
        SET_GREY_TEXT;
        if (music_on) {
            render_text(renderer, txt_font, "Music ", screen_w/2, board_h*TILE_SIZE, ALIGN_RIGHT, ALIGN_TOP);
        }
        if (sfx_on) {
            render_text(renderer, txt_font, "  SFX", screen_w/2, board_h*TILE_SIZE, ALIGN_LEFT, ALIGN_TOP);
        }
        SDL_RenderPresent(renderer);
    }

    SDL_RenderPresent(renderer);
}


/*############ MAIN GAME LOOP #############*/
int main(int argc, char ** argv)
{
    SDL_Event event;

    game_state = NotInitialized;
    if (init_game_engine(screen_w, screen_h, fullscreen)) return 1;

    game_state = TitleScreen; // Initialize to TitleScreen
    if (music_on) Mix_PlayMusic(idle_music, -1);

    /* Start timer for generating SDL_USEREVENT events for screen frame rendering */
    game_timer = SDL_AddTimer(RENDER_INTERVAL, tick_callback, 0);

    while (game_state != NotInitialized) { /* main game loop */
        while (SDL_PollEvent(&event)) { /* process pending events */
            switch (event.type) {
            case SDL_USEREVENT: /* next frame event */
                // Update game state at a fixed rate
                if (game_state == Playing || game_state == GameOver) {
                    frame++;
                }
                if (game_state == Playing) {
                    if (frame % CHAR_ANIM_FRAMES == 0) {
                        update_play_state();
                    }
                // Calculate animation progress between updates
                    animation_progress = (frame % CHAR_ANIM_FRAMES)/(double)CHAR_ANIM_FRAMES;
                }
                // Always render, regardless of state, to show menus or game
                render_screen();
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int mouse_x = event.button.x;
                    int mouse_y = event.button.y;
                    if (game_state == TitleScreen) {
                        for (int i = 0; i < title_menu_item_count; i++) {
                            if (mouse_x >= title_menu_items[i].rect.x && mouse_x <= title_menu_items[i].rect.x + title_menu_items[i].rect.w &&
                                mouse_y >= title_menu_items[i].rect.y && mouse_y <= title_menu_items[i].rect.y + title_menu_items[i].rect.h) {
                                title_menu_selected_item = i; // Visually select
                                if (title_menu_items[i].action) title_menu_items[i].action();
                                break;
                            }
                        }
                    } else if (game_state == OptionsMenu) {
                        for (int i = 0; i < options_menu_item_count; i++) {
                            if (mouse_x >= options_menu_items[i].rect.x && mouse_x <= options_menu_items[i].rect.x + options_menu_items[i].rect.w &&
                                mouse_y >= options_menu_items[i].rect.y && mouse_y <= options_menu_items[i].rect.y + options_menu_items[i].rect.h) {
                                options_menu_selected_item = i; // Visually select
                                if (options_menu_items[i].is_key_config) {
                                    configuring_key_item_index = i;
                                    menu_action_configure_key(&options_menu_items[i]);
                                } else if (options_menu_items[i].action) {
                                    options_menu_items[i].action();
                                }
                                break;
                            }
                        }
                    }
                }
                break;
            case SDL_KEYDOWN:
                ck_press = 0; // Reset control key press indicator for game state
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (game_state == KeyConfiguring) {
                        game_state = OptionsMenu;
                        configuring_key_item_index = -1;
                    } else if (game_state == OptionsMenu) {
                        menu_action_back_to_title();
                    } else {
                        menu_action_exit();
                    }
                    break;
                }
                if (game_state == KeyConfiguring) {
                    // Check for conflicts
                    SDL_KeyCode pressed_key = event.key.keysym.sym;
                    int conflict = 0;
                    if (pressed_key == key_left && options_menu_items[configuring_key_item_index].key_to_configure != &key_left) conflict = 1;
                    else if (pressed_key == key_right && options_menu_items[configuring_key_item_index].key_to_configure != &key_right) conflict = 1;
                    else if (pressed_key == key_up && options_menu_items[configuring_key_item_index].key_to_configure != &key_up) conflict = 1;
                    else if (pressed_key == key_down && options_menu_items[configuring_key_item_index].key_to_configure != &key_down) conflict = 1;
                    else if (pressed_key == key_pause && options_menu_items[configuring_key_item_index].key_to_configure != &key_pause) conflict = 1;
                    
                    // Also check against other menu navigation/activation keys if necessary
                    if (pressed_key == SDLK_RETURN || pressed_key == SDLK_KP_ENTER || pressed_key == SDLK_SPACE ||
                        pressed_key == SDLK_UP || pressed_key == SDLK_DOWN) {
                        conflict = 1;
                    }

                    if (!conflict && configuring_key_item_index != -1) {
                        *(options_menu_items[configuring_key_item_index].key_to_configure) = pressed_key;
                    }
                    game_state = OptionsMenu;
                    configuring_key_item_index = -1;
                } else if (game_state == TitleScreen) {
                    switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        title_menu_selected_item = (title_menu_selected_item - 1 + title_menu_item_count) % title_menu_item_count;
                        break;
                    case SDLK_DOWN:
                        title_menu_selected_item = (title_menu_selected_item + 1) % title_menu_item_count;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                    case SDLK_SPACE:
                        if (title_menu_items[title_menu_selected_item].action) {
                            title_menu_items[title_menu_selected_item].action();
                        }
                        break;
                    default:
                        break;
                    }
                } else if (game_state == OptionsMenu) {
                    switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        options_menu_selected_item = (options_menu_selected_item - 1 + options_menu_item_count) % options_menu_item_count;
                        break;
                    case SDLK_DOWN:
                        options_menu_selected_item = (options_menu_selected_item + 1) % options_menu_item_count;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                    case SDLK_SPACE:
                        if (options_menu_selected_item == OPTIONS_IDX_WIDTH && !fullscreen) {
                            game_state = EditingBoardWidth;
                            board_dim_input[0] = 0;
                            editing_field = 0;
                        } else if (options_menu_selected_item == OPTIONS_IDX_HEIGHT && !fullscreen) {
                            game_state = EditingBoardHeight;
                            board_dim_input[0] = 0;
                            editing_field = 1;
                        } else if (options_menu_items[options_menu_selected_item].is_key_config) {
                            configuring_key_item_index = options_menu_selected_item;
                            menu_action_configure_key(&options_menu_items[options_menu_selected_item]);
                        } else if (options_menu_items[options_menu_selected_item].action) {
                            options_menu_items[options_menu_selected_item].action();
                        }
                        break;
                    default:
                        break;
                    }
                } else if (game_state == Playing) {
                    if (event.key.keysym.sym == key_pause) {
                        pause_play();
                    } else if (event.key.repeat == 0 && ck_press == 0) { // ck_press ensures one direction change per key press
                        if (event.key.keysym.sym == key_left) { if (dhx == 0) { dhx = -1; dhy = 0; ck_press = 1; } }
                        else if (event.key.keysym.sym == key_right) { if (dhx == 0) { dhx = 1; dhy = 0; ck_press = 1; } }
                        else if (event.key.keysym.sym == key_up) { if (dhy == 0) { dhx = 0; dhy = -1; ck_press = 1; } }
                        else if (event.key.keysym.sym == key_down) { if (dhy == 0) { dhx = 0; dhy = 1; ck_press = 1; } }
                    }
                } else if (game_state == Paused) {
                    if (event.key.keysym.sym == key_pause) {
                        resume_play();
                    }
                } else if (game_state == GameOver) {
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        game_state = TitleScreen; // Go to title screen
                        title_menu_selected_item = 0;
                        if (music_on && Mix_PlayingMusic() == 0) { // Resume idle music if it stopped
                            Mix_PlayMusic(idle_music, -1);
                        }
                    }
                } else if (game_state == EditingBoardWidth || game_state == EditingBoardHeight) {
                    // Only allow digits and backspace
                    SDL_Keycode sym = event.key.keysym.sym;
                    int len = strlen(board_dim_input);
                    if (sym >= SDLK_0 && sym <= SDLK_9 && len < 6) {
                        board_dim_input[len] = '0' + (sym - SDLK_0);
                        board_dim_input[len+1] = 0;
                    } else if (sym == SDLK_BACKSPACE && len > 0) {
                        board_dim_input[len-1] = 0;
                    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                        int val = atoi(board_dim_input);
                        int valid = 0;
                        if (game_state == EditingBoardWidth) {
                            if (val >= MIN_BOARD_WIDTH) { win_board_w = val; valid = 1; }
                        } else if (game_state == EditingBoardHeight) {
                            if (val >= MIN_BOARD_HEIGHT) { win_board_h = val; valid = 1; }
                        }
                        // If valid, apply and resize window
                        if (valid && !fullscreen) {
                            create_windowed_display();
                            // Destroy and recreate the game board texture
                            if (txt_game_board) SDL_DestroyTexture(txt_game_board);
                            txt_game_board = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_TARGET, screen_w, screen_h);
                            if (game_board) free(game_board);
                            game_board = calloc(board_w * board_h, sizeof(BoardField));
                            init_game_board();
                        }
                        // Return to options menu
                        game_state = OptionsMenu;
                        board_dim_input[0] = 0;
                        editing_field = -1;
                    } else if (sym == SDLK_ESCAPE) {
                        // Cancel editing
                        game_state = OptionsMenu;
                        board_dim_input[0] = 0;
                        editing_field = -1;
                    }
                    break;
                }
            case SDL_QUIT: /* window closed */
                game_state = NotInitialized;
                break;
            default: /* ignore other events */
                break;
            }
        }
        SDL_Delay(10); /* loop limiter, give some CPU cycles to other processes */
    }

    cleanup_game();
    return 0;
}

// Implementation of menu actions
void menu_action_play(void) {
    start_play();
}

void menu_action_options(void) {
    game_state = OptionsMenu;
    options_menu_selected_item = 0; // Reset selection when entering options
}

void menu_action_exit(void) {
    game_state = NotInitialized; // This will lead to cleanup and exit in main loop
}

void menu_action_back_to_title(void) {
    game_state = TitleScreen;
    title_menu_selected_item = 0; // Reset selection
}

void menu_action_toggle_music(void) {
    music_on = !music_on;
    if (music_on) {
        if (game_state == TitleScreen || game_state == OptionsMenu) Mix_PlayMusic(idle_music, -1);
        else if (game_state == Playing) Mix_PlayMusic(play_music, -1);
    } else {
        Mix_HaltMusic();
    }
}

void menu_action_toggle_sfx(void) {
    sfx_on = !sfx_on;
}

void menu_action_toggle_fullscreen(void) {
    fullscreen = !fullscreen;
    
    // Store current game state
    GameState prev_state = game_state;
    
    // Temporarily set to NotStarted to allow proper reinitialization
    game_state = NotStarted;
    
    // Create display area based on new mode
    if (fullscreen) {
        if (create_fullscreen_display()) return;
    } 
    else if (create_windowed_display()) return;
    
    // Destroy and recreate the game board texture
    if (txt_game_board) {
        SDL_DestroyTexture(txt_game_board);
    }
    txt_game_board = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_TARGET, screen_w, screen_h);
    if (!txt_game_board) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error creating background texture", SDL_GetError(), NULL);
        return;
    }
    
    // Reallocate game board array if needed
    if (game_board) {
        free(game_board);
    }
    game_board = calloc(board_w * board_h, sizeof(BoardField));
    if (!game_board) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error", "Error allocating memory", NULL);
        return;
    }
    
    // Reinitialize the game board
    init_game_board();
    
    // Restore previous game state
    game_state = prev_state;
}

void menu_action_configure_key(MenuItem* item) {
    if (item && item->is_key_config) {
        game_state = KeyConfiguring;
        // The actual key configuration will be handled in the event loop for KeyConfiguring state
    }
}
