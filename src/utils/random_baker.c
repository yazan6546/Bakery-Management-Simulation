#include "random.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void init_random() {
    srand(time(NULL) ^ getpid());
}

float random_float(float min, float max) {
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}
