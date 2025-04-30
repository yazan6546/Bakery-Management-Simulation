#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdarg.h>

#include "config.h"
#include "inventory.h"
#include "oven.h"

#define MAX_OVENS 10  // Max ovens allowed

typedef struct Game {
    int elapsed_time;
    int num_frustrated_customers;
    int num_complained_customers;
    int num_customers_missing;
    float daily_profit;

    Config config;
    Inventory inventory;

    Oven ovens[MAX_OVENS]; // Shared ovens
} Game;

// Still can keep these (but optional now)
pid_t start_process(const char *binary, int shared_mem_fd);
int game_init(Game *game, pid_t *processes, int shared_mem_fd);
void game_destroy(int shm_fd, Game *shared_game);
void game_create(int *shm_fd, Game **shared_game);
int check_game_conditions(const Game *game);
void print_with_time1(const Game *game, const char *format, ...);

#endif // GAME_H
