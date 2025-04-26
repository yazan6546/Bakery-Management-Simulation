//
// Created by - on 3/26/2025.
//

#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include "player.h"
#include "config.h"
#include "inventory.h"
#include <stdarg.h>

#define TEAM_SIZE 4

typedef struct Game {
    int elapsed_time;
    int num_frustrated_customers;
    int num_complained_customers;
    int num_customers_missing;
    Config config;
    Inventory inventory;
} Game;

pid_t start_process(const char *binary, Config *config);
void init_game(Game *game);
void game_destroy(int shm_fd, Game *shared_game);
void game_create(int *shm_fd, Game **shared_game);
Team simulate_round(int pipe_fds_team_A[], int pipe_fds_team_B[], const Config *config, Game *game);
int check_game_conditions(const Game *game, const Config *config, Team team_win);
void go_to_next_round(Game *game);
int check_round_conditions(const Game *game, const Config *config);
void print_with_time1(const Game *game, const char *format, ...);

#endif //GAME_H
