#ifndef SHARED_MEM_UTILS_H
#define SHARED_MEM_UTILS_H

#include "game.h"


void setup_shared_memory(Game **shared_game);

void cleanup_shared_memory(Game *shared_game);


#endif // SHARED_MEM_UTILS_H