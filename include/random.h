//
// Created by - on 3/23/2025.
//

#ifndef RANDOM_H
#define RANDOM_H

#include <stdlib.h>
#include <time.h>
#include "config.h"
#include "player.h"

void init_random();
float random_float(float min, float max);
void randomize_attributes(Attributes *attributes, Config *configs);

#endif //RANDOM_H
