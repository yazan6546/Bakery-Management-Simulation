#ifndef SHARED_MEM_UTILS_H
#define SHARED_MEM_UTILS_H
#define GAME_SHM_NAME "/game_shared_mem"
#define CUSTOMER_QUEUE_SHM_NAME "/customer_queue_shm"

#include "game.h"
#include "queue.h"


int setup_shared_memory(Game **shared_game);
void cleanup_shared_memory(Game *shared_game);

void cleanup_queue_shared_memory(queue_shm *queue_shm, size_t capacity);
void setup_queue_shared_memory(queue_shm **queue_shm, size_t capacity);




#endif // SHARED_MEM_UTILS_H