#ifndef HISCORES_H
#define HISCORES_H

#include "types.h"

void hiscores_init(void);
int hiscores_is_highscore(int score);
void hiscores_add(const char *player_name, int score, int board_width, int board_height);
void hiscores_load(void);
void hiscores_save(void);
const Hiscore* hiscores_get_scores(void);
void hiscores_clear(void);

#endif // HISCORES_H
