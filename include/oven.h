#ifndef OVEN_H
#define OVEN_H

#include <semaphore.h>

#define MAX_OVENS 10  // adjust as needed

typedef struct {
    int id;
    int is_busy;
    int time_left;
    char item_name[50];
    char team_name[50];
} Oven;

// Oven control functions
void init_oven(Oven *oven, int id);
int put_item_in_oven(Oven *oven, const char *item_name, const char *team_name, int baking_time);
int oven_tick(Oven *oven);

// Semaphore-based synchronization
int setup_oven_semaphores(int num_ovens);
void lock_oven(int oven_id);
void unlock_oven(int oven_id);
void cleanup_oven_semaphores(int num_ovens);  // optional cleanup

#endif // OVEN_H
