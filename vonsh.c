#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCR_W 1024
#define SCR_H 768
#define TILE_SIZE 16
#define GAME_SPEED 200

SDL_Window *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *srf_ground = NULL;
SDL_Texture *txt_ground = NULL;
SDL_Surface *srf_snake_body = NULL;
SDL_Texture *txt_snake_body = NULL;
SDL_Surface *srf_snake_head = NULL;
SDL_Texture *txt_snake_head = NULL;
SDL_Surface *srf_food = NULL;
SDL_Texture *txt_food = NULL;
SDL_Surface *srf_wall = NULL;
SDL_Texture *txt_wall = NULL;
SDL_TimerID game_timer = 0;

const int game_board_w = SCR_W/TILE_SIZE, game_board_h = SCR_H/TILE_SIZE;
int dhx=0, dhy=0; /* head movement direction */
int hx=0, hy=0; /* head coordinates */
int tx=0, ty=0; /* tail coordinates */
int score=0;
int expand_counter=0;
int board_offset=0;

typedef enum e_FieldType { /* indicates what is on game board field */
    Empty,
    Snake,
    Food,
    Wall
} FieldType;

typedef struct BoardField {
    FieldType type; /* type of this field */
    int dx; /* delta x to next piece of snake. 0 if head */
    int dy; /* delta y to next piece of snake. 0 if head */
} BoardField;
BoardField *game_board = NULL;

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
            break;
        }
    }
}

void render_screen(void)
{
    int x, y;
    static SDL_Rect DstR = { 0, 0, TILE_SIZE, TILE_SIZE };
    SDL_RenderClear(renderer);
    for (y=0; y<game_board_h; y++)
        for (x=0; x<game_board_w; x++) {
            DstR.x = x*TILE_SIZE; DstR.y = y*TILE_SIZE;
            SDL_RenderCopy(renderer, txt_ground, NULL, &DstR);
            switch(game_board[game_board_w*y+x].type) {
                case Snake:
                    if (x == hx && y == hy)
                        SDL_RenderCopy(renderer, txt_snake_head, NULL, &DstR);
                    else
                        SDL_RenderCopy(renderer, txt_snake_body, NULL, &DstR);
                    break;
                case Food:
                    SDL_RenderCopy(renderer, txt_food, NULL, &DstR);
                    break;
                case Wall:
                    SDL_RenderCopy(renderer, txt_wall, NULL, &DstR);
                    break;
                default:
                    break;
            }
        }
    SDL_RenderPresent(renderer);
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
    screen = SDL_CreateWindow("Snake game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_W, SCR_H, 0);
    renderer = SDL_CreateRenderer(screen, -1, 0);
    srf_ground = IMG_Load("resources/tile_ground.png");
    txt_ground = SDL_CreateTextureFromSurface(renderer, srf_ground);
    srf_snake_body = IMG_Load("resources/tile_snake_body.png");
    txt_snake_body = SDL_CreateTextureFromSurface(renderer, srf_snake_body);
    srf_snake_head = IMG_Load("resources/tile_snake_head.png");
    txt_snake_head = SDL_CreateTextureFromSurface(renderer, srf_snake_head);
    srf_food = IMG_Load("resources/tile_food.png");
    txt_food = SDL_CreateTextureFromSurface(renderer, srf_food);
    srf_wall = IMG_Load("resources/tile_wall.png");
    txt_wall = SDL_CreateTextureFromSurface(renderer, srf_wall);

    /* initialize movement direction and position */
    score = expand_counter = 0;
    dhx = 0;   dhy = -1;
    hx = tx = SCR_W/(TILE_SIZE*2);   hy = ty = SCR_H/(TILE_SIZE*2);
    game_board[game_board_w*hy+hx].type = Snake;
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
                /* main game logic */
                if (hx+dhx < 0             || hy+dhy < 0 ||
                    hx+dhx >= game_board_w || hy+dhy >= game_board_h) {
                    /* collision with game board edge */
                    quit = 1; /* GAME OVER */
                    break;
                }
                else if (game_board[game_board_w*(hy+dhy)+hx+dhx].type == Food) {
                    score++;
                    expand_counter += score;
                    seed_item(Food);
                }
                else if (game_board[game_board_w*(hy+dhy)+hx+dhx].type != Empty) {
                    /* collision with snake body or wall */
                    quit = 1; /* GAME OVER */
                    break;
                }
                /* move snake head */
                game_board[game_board_w*hy+hx].dx = dhx;
                game_board[game_board_w*hy+hx].dy = dhy;
                hx += dhx;    hy += dhy;
                game_board[game_board_w*hy+hx].type = Snake;
                /* move snake tail */
                if (expand_counter==0) {
                    board_offset = game_board_w*ty+tx;
                    tx = tx + game_board[board_offset].dx;
                    ty = ty + game_board[board_offset].dy;
                    game_board[board_offset].type = Empty;
                    game_board[board_offset].dx = 0;
                    game_board[board_offset].dy = 0;
                }
                else {
                    seed_item(Wall);
                    expand_counter--;
                }

                render_screen();
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                else if (event.key.repeat == 0) {
                    switch (event.key.keysym.sym) {
                        case SDLK_LEFT:  if (dhx == 0) { dhx = -1; dhy = 0; } break;
                        case SDLK_RIGHT: if (dhx == 0) { dhx = 1; dhy = 0; } break;
                        case SDLK_UP:    if (dhy == 0) { dhx = 0; dhy = -1; } break;
                        case SDLK_DOWN:  if (dhy == 0) { dhx = 0; dhy = 1; } break;
                    }
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
        }
    }

    SDL_RemoveTimer(game_timer);

    SDL_DestroyTexture(txt_wall);
    SDL_DestroyTexture(txt_food);
    SDL_DestroyTexture(txt_snake_head);
    SDL_DestroyTexture(txt_snake_body);
    SDL_DestroyTexture(txt_ground);
    SDL_FreeSurface(srf_wall);
    SDL_FreeSurface(srf_food);
    SDL_FreeSurface(srf_snake_head);
    SDL_FreeSurface(srf_snake_body);
    SDL_FreeSurface(srf_ground);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    IMG_Quit();
    SDL_Quit();

    free(game_board);

    return 0;
}
