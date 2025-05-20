#ifndef TYPES_H
#define TYPES_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>

#define RES_DIR "../share/games/vonsh/" /* resources directory */
#define USER_SHARE_DIR "~/.local/share/vonsh/"
#define TOP_SCORES_FILE "top_scores_"VERSION_STR".json"
#define INIT_CONFIG_FILE "config.json" /* configuration file name */
#define USER_CONFIG_FILE "config_"VERSION_STR".json" /* configuration file name */
#define WINDOW_TITLE "Vonsh" /* window title string */
#define BOARD_MIN_WIDTH (28) /* minimum board width in tiles */
#define BOARD_MIN_HEIGHT (28) /* minimum board height in tiles */
#define TILE_SIZE (16) /* tile side in pixels */
#define RENDER_INTERVAL (50)   // Interval between frames in milliseconds
#define GROUND_TILES (8)  // Number of ground tiles in the tileset
#define WALL_TILES (4)  // Number of wall tiles in the tileset
#define FOOD_TILES (6)  // Number of food tiles in the tileset
#define FOOD_BLINK_FRAMES (27)
#define CHAR_ANIM_FRAMES (4) // Number of animation frames for the character
#define TOTAL_CHARS (24) // Number of total characters
#define MAX_HISCORES (10)
#define MAX_NAME_LEN (15)
#define MENU_WIDTH (20*TILE_SIZE) /* menu width in pixels */
#define MENU_BORDER (20) /* menu border in pixels */
#define MENU_LOGO_SPACE (8) /* logo space in pixels */
#define MENU_HEAD_SPACE (30) /* head space in pixels */
#define MENU_ITEM_WIDTH (180) /* menu item width in pixels */
#define MENU_ITEM_HEIGHT (24) /* menu item height in pixels */
#define GAME_OVER_WIDTH (16*TILE_SIZE) /* game over overlay width in pixels */
#define GAME_OVER_BORDER (16) /* game over overlay border in pixels */
#define GAME_OVER_ITEM_SPACE (12) /* game over overlay item space in pixels */
#define FPS_COUNT_INTERVAL (1000) /* interval between FPS counter updates in milliseconds */
//REMARK: despite that there are only 3 different animation frames per character per each direction, animation cycle CHAR_ANIM_FRAMES has 4 frames because one of the frames is shown twice in this cycle


typedef enum e_GameState {
    NotInitialized,
    Playing,
    Paused,
    GameOver,
    EnteringHiscoreName,
    MainMenu,
    OptionsMenu,
    HallOfFame
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
    int pdx; /* delta x to previous piece of snake. */
    int pdy; /* delta y to previous piece of snake. */
} BoardField;

typedef struct {
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_Texture *txt_game_board;
    SDL_Texture *txt_env_tileset;
    SDL_Texture *txt_food_tileset;
    SDL_Texture *txt_food_marker;
    SDL_Texture *txt_char_tileset;
    SDL_Texture *txt_font;
    SDL_Texture *txt_logo;
    SDL_Texture *txt_trophy;
    SDL_Rect *ground_tile;
    SDL_Rect *wall_tile;
    SDL_Rect *food_tile;
} Graphics;

typedef struct {
    Mix_Chunk *exp_chunk;
    Mix_Chunk *die_chunk;
    Mix_Music *idle_music;
    Mix_Music *gameplay_music;
} Audio;

typedef struct {
    int window_board_w;
    int window_board_h;
    int fullscreen_board_w;
    int fullscreen_board_h;
    int *current_board_w;
    int *current_board_h;
    int window_w;
    int window_h;
    bool fullscreen;
    int dhx, dhy;
    int hx, hy; //head position
    int tx, ty; //tail position
    int score;
    int hi_score;
    bool new_record;
    int expand_counter;
    int ck_press;
    int frame;
    float animation_progress;
    bool music_on;
    bool sfx_on;
    BoardField *game_board;
    GameState state;
    char player_name[MAX_NAME_LEN + 1];
    int player_name_len;
    SDL_TimerID game_timer;
    SDL_KeyCode key_left;
    SDL_KeyCode key_right;
    SDL_KeyCode key_up;
    SDL_KeyCode key_down;
    SDL_KeyCode key_pause;
    bool fps_counter_on;
    int fps;
} Game;

typedef enum e_TextHorizontalAlignment {
    ALIGN_LEFT,
    ALIGN_CENTER_HORIZONTAL,
    ALIGN_RIGHT
} TextHorizontalAlignment;

typedef enum e_TextVerticalAlignment {
    ALIGN_TOP,
    ALIGN_CENTER_VERTICAL,
    ALIGN_BOTTOM
} TextVerticalAlignment;

typedef enum e_TextColor {
    TEXT_WHITE,
    TEXT_GREY,
    TEXT_YELLOW
} TextColor;

typedef struct s_TableRow {
    char** cells;
    int num_cells;
    TextColor color;
} TableRow;

typedef struct s_Table {
    TableRow* rows;
    int num_rows;
} Table;


typedef enum e_MenuItemType {
    MenuItemType_Label,
    MenuItemType_KeyConfig,
    MenuItemType_IntConfig,
    MenuItemType_Switch,
    MenuItemType_TableRow
} MenuItemType;

typedef struct {
    TextColor color;
} LabelInfo;

typedef struct {
    SDL_KeyCode* key_code;
} KeyConfigInfo;

typedef struct {
    int** value;
    int min_value;
} IntConfigInfo;

typedef struct {
    bool* value;
    const char* const* states;
} SwitchInfo;

typedef struct {
    TableRow* row;
} TableRowInfo;

typedef struct MenuItem {
    MenuItemType type;
    const char* label;
    void (*action)(struct MenuItem *item);
    SDL_Rect rect;
    bool active;
    union {
        LabelInfo label_info;
        KeyConfigInfo key_config_info;
        IntConfigInfo int_config_info;
        SwitchInfo switch_info;
        TableRowInfo table_row_info;
    } data;
} MenuItem;

typedef struct Menu {
    int count;
    MenuItem *items;
} Menu;

typedef struct {
    char player_name[MAX_NAME_LEN + 1];
    int score;
    time_t date;
    int board_width;
    int board_height;
} Hiscore;


extern Graphics g_gfx;
extern Game g_game;

#endif // TYPES_H
