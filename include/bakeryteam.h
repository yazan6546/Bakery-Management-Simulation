#ifndef BAKERYTEAM_H
#define BAKERYTEAM_H

#include "game.h"

void handle_bread_team(int mqid_from_main, int mqid_ready, Game *game);
void handle_cakesweets_team(int mqid_from_main, int mqid_ready, Game *game);
void handle_patisseries_team(int mqid_from_main, int mqid_ready, Game *game);

#endif // BAKERYTEAM_H
