#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef enum e_FieldType { /* indicates what is inside game board field */
    Empty,
    Snake,
    Food,
    Wall
} FieldType;

typedef struct BoardField {
    FieldType type; /* type of this field */
    int p; /* paremeter(kind of character in snake body, kind of food, kind of wall ) */
    int dx; /* delta x to next piece of snake. 0 if head */
    int dy; /* delta y to next piece of snake. 0 if head */
} BoardField;

#define SCR_W 1024
#define SCR_H 768
#define TILE_SIZE 16
#define GAME_SPEED 200
#define GROUND_TILES 8
#define WALL_TILES 4
#define FOOD_TILES 6
#define TOTAL_CHAR 12
SDL_Window *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *txt_game_board = NULL;
SDL_Surface *srf_env_tileset = NULL;
SDL_Texture *txt_env_tileset = NULL;
SDL_Surface *srf_food_tileset = NULL;
SDL_Texture *txt_food_tileset = NULL;
SDL_Surface *srf_char_tileset = NULL;
SDL_Texture *txt_char_tileset = NULL;
SDL_TimerID game_timer = 0;

SDL_Rect ground_tile[GROUND_TILES];
SDL_Rect wall_tile[WALL_TILES];
SDL_Rect food_tile[FOOD_TILES];
BoardField *game_board = NULL;

const int game_board_w = SCR_W/TILE_SIZE, game_board_h = SCR_H/TILE_SIZE;
int dhx=0, dhy=0;     /* head movement direction */
int hx=0, hy=0;       /* head coordinates */
int tx=0, ty=0;       /* tail coordinates */
int score=0;          /* player's score, number of food eaten */
int expand_counter=0; /* counter of snake segments to add and walls to seed */
int ck_press=0;       /* control key pressed indicator */

void init_tiles(void) {
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
}

void init_game_board(void) {
    /* Redirect rendering to the game board texture */
    SDL_SetRenderTarget(renderer, txt_game_board);
    SDL_RenderClear(renderer);

    /* Fill background with random pattern of ground tiles */
    int x, y;
    SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    SDL_RenderClear(renderer);
    for (y=0; y<game_board_h; y++)
        for (x=0; x<game_board_w; x++) {
            DstR.x = x*TILE_SIZE; DstR.y = y*TILE_SIZE;
            SDL_RenderCopy(renderer, txt_env_tileset, &ground_tile[rand()%GROUND_TILES], &DstR);
        }

    /* Detach the game board texture from renderer */
    SDL_SetRenderTarget(renderer, NULL);
}

void seed_item(FieldType item_type)
{
    int x = 0, y = 0; /* coordinates */
    while(1)
    {
        x = rand() % game_board_w;
        y = rand() % game_board_h;
        if (game_board[game_board_w*y+x].type == Empty)
        {
            game_board[game_board_w*y+x].type = item_type;

            /* render seeded wall to game board texture */
            if (item_type == Wall) {
                SDL_Rect DstR = { x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE };
                /* Redirect rendering to the game board texture */
                SDL_SetRenderTarget(renderer, txt_game_board);
                /* Copy wall tile */
                SDL_RenderCopy(renderer, txt_env_tileset, &wall_tile[rand()%WALL_TILES], &DstR);
                /* Detach the game board texture from renderer */
                SDL_SetRenderTarget(renderer, NULL);
            }
            else if (item_type == Food) {
                game_board[game_board_w*y+x].p = rand()%FOOD_TILES;
            }
            break;
        }
    }
}

void render_screen(void)
{
    /* every screen refresh everything is rendered from scratch */
    int x, y;
    int foff; /* game board field offset */
    SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    SDL_Rect SrcR = { 0, 0, TILE_SIZE, TILE_SIZE };
    /* render game backround with walls */
    SDL_RenderCopy(renderer, txt_game_board, NULL, NULL);

    /* render snake body and food*/
    for (y=0; y<game_board_h; y++) {
        DstR.y = y*TILE_SIZE;
        for (x=0; x<game_board_w; x++) {
            DstR.x = x*TILE_SIZE;
            foff = game_board_w*y+x;
            switch(game_board[foff].type) {
                case Snake:
                    if (game_board[foff].dx == 0 && game_board[foff].dy == 1) {
                        SrcR.x = 0;
                    }
                    else if (game_board[foff].dx == 1 && game_board[foff].dy == 0) {
                        SrcR.x = TILE_SIZE;
                    }
                    else if (game_board[foff].dx == 0 && game_board[foff].dy == -1) {
                        SrcR.x = 2*TILE_SIZE;
                    }
                    else if (game_board[foff].dx == -1 && game_board[foff].dy == 0) {
                        SrcR.x = 3*TILE_SIZE;
                    }
                    SrcR.y = game_board[foff].p*3*(TILE_SIZE+1) + 1;
                    SDL_RenderCopy(renderer, txt_char_tileset, &SrcR, &DstR);
                    break;
                case Food:
                    SDL_RenderCopy(renderer, txt_food_tileset,
                                   &food_tile[game_board[foff].p], &DstR);
                    break;
                default:
                    break;
            }
        }
    }
    SDL_RenderPresent(renderer);
}

/* one iteration of game logic
    retval:
    0 - game continues
    1 - game over
 */
int game_logic(void)
{
    /* verify all kinds of collisions */
    if (hx+dhx < 0             || hy+dhy < 0 ||
        hx+dhx >= game_board_w || hy+dhy >= game_board_h) {
        /* collision with game board edge */
        return 1; /* GAME OVER */
    }
    else if (game_board[game_board_w*(hy+dhy)+hx+dhx].type == Food) {
        /* food hit - increase score, start expanding snake, seed new food */
        score++;
        expand_counter += score;
        seed_item(Food);
    }
    else if (game_board[game_board_w*(hy+dhy)+hx+dhx].type != Empty) {
        /* collision with snake body or wall */
        return 1; /* GAME OVER */
    }
    int foff; /* game board field offset */
    /* move snake head */
    foff = game_board_w*hy+hx;
    game_board[foff].dx = dhx; /* "neck" direction */
    game_board[foff].dy = dhy;
    hx += dhx;    hy += dhy;
    foff = game_board_w*hy+hx;
    game_board[foff].type = Snake;
    game_board[foff].dx = dhx; /* head direction */
    game_board[foff].dy = dhy;

    /* shift character kinds from tail to head */
    int ix = tx, iy = ty;
    int pb1, pb2; /* buffers for shifting character kinds from tail to head */
    pb1 = game_board[game_board_w*iy+ix].p;
    do {
        foff = game_board_w*iy+ix;
        ix = ix + game_board[foff].dx;
        iy = iy + game_board[foff].dy;
        foff = game_board_w*iy+ix;
        pb2 = game_board[foff].p;
        game_board[foff].p = pb1;
        pb1 = pb2;
    } while(ix!=hx || iy!=hy);

    if (expand_counter==0) { /* move snake tail if not expanding */
        foff = game_board_w*ty+tx; /* tail offset on game board */
        tx = tx + game_board[foff].dx;
        ty = ty + game_board[foff].dy;
        game_board[foff].type = Empty;
        game_board[foff].p = 0;
        game_board[foff].dx = 0;
        game_board[foff].dy = 0;
    }
    else { /* expand tail and seed new obstacle */
        game_board[game_board_w*ty+tx].p = rand()%TOTAL_CHAR;
        seed_item(Wall);
        expand_counter--;
    }
    return 0;
}

uint32_t tick_callback(uint32_t interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;
    userevent.type = SDL_USEREVENT;  userevent.code = 0;
    userevent.data1 = NULL;  userevent.data2 = NULL;
    event.type = SDL_USEREVENT;  event.user = userevent;
    SDL_PushEvent(&event);
    return interval;
}

int main(int argc, char ** argv)
{
    int quit = 0;
    SDL_Event event;

    /* allocate and initialize game board */
    game_board = calloc(game_board_w*game_board_h, sizeof(BoardField));
    for (int i=0; i<game_board_w*game_board_h; i++) {
        game_board[i].type = Empty;
        game_board[i].dx = 0;
        game_board[i].dy = 0;
    }

    /* init SDL library and load game resources */
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    IMG_Init(IMG_INIT_PNG);
    screen = SDL_CreateWindow("Vonsh", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_W, SCR_H, 0);
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    /* texture for rendering game background with obstacles */
    txt_game_board = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCR_W, SCR_H);

    /* textures with game assets */
    srf_env_tileset = IMG_Load("resources/board_tiles.png");
    txt_env_tileset = SDL_CreateTextureFromSurface(renderer, srf_env_tileset);
    srf_food_tileset = IMG_Load("resources/food_tiles.png");
    txt_food_tileset = SDL_CreateTextureFromSurface(renderer, srf_food_tileset);
    srf_char_tileset = IMG_Load("resources/character_tiles.png");
    txt_char_tileset = SDL_CreateTextureFromSurface(renderer, srf_char_tileset);

    /* initialize environment tileset info */
    init_tiles();

    init_game_board();

    /* initialize movement direction and position */
    score = expand_counter = 0;
    dhx = 0;   dhy = -1;
    hx = tx = SCR_W/(TILE_SIZE*2);   hy = ty = SCR_H/(TILE_SIZE*2);
    game_board[game_board_w*hy+hx].type = Snake;
    game_board[game_board_w*hy+hx].p = rand()%TOTAL_CHAR;
    game_board[game_board_w*hy+hx].dx = dhx;
    game_board[game_board_w*hy+hx].dy = dhy;
    seed_item(Food);
    render_screen();

    /* init timer and trigger rendering of first frame */
    game_timer = SDL_AddTimer(GAME_SPEED, tick_callback, 0);

    while (!quit)
    {
        SDL_WaitEvent(&event);
        switch (event.type)
        {
            case SDL_USEREVENT: /* game tick event */
                if (game_logic()) {
                    quit = 1; /* GAME OVER, no need to render next frame */
                }
                else { /* game continues */
                    render_screen(); /* render next frame */
                    ck_press = 0; /* allow new control key press */
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                else if (event.key.repeat == 0 && ck_press == 0) {
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

    SDL_RemoveTimer(game_timer);

    SDL_DestroyTexture(txt_char_tileset);
    SDL_DestroyTexture(txt_env_tileset);
    SDL_DestroyTexture(txt_food_tileset);
    SDL_DestroyTexture(txt_game_board);

    SDL_FreeSurface(srf_char_tileset);
    SDL_FreeSurface(srf_env_tileset);
    SDL_FreeSurface(srf_food_tileset);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    IMG_Quit();
    SDL_Quit();

    free(game_board);

    return 0;
}
