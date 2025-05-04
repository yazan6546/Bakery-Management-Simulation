//
// Created by - on 3/26/2025.
//

#include "game.h"
#include "inventory.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "config.h"
#include "unistd.h"
#include <string.h>
#include <stdbool.h>
#include "shared_mem_utils.h"


int game_init(Game *game, pid_t *processes, pid_t *processes_sellers, int shared_mem_fd) {

    game->elapsed_time = 0;
    game->num_frustrated_customers = 0;
    game->num_complained_customers = 0;
    game->num_customers_missing = 0;
    game->num_customers_served = 0;
    game->num_customers_cascade = 0;
    game->daily_profit = 0.0f;
    game->complaining_customer_pid = 0;
    game->recent_complaint = false;
    init_inventory(&game->inventory);


    char *binary_paths[] = {
        "./graphics",
        "./chefs",
        "./bakers",
        "./supply_chain_manager",
        "./customer_manager"
    };

    for (int i = 0; i < 5; i++) {
        bool suppress;
        suppress = true;
        if (strcmp(binary_paths[i], "./customer_manager") == 0)
            suppress = false;
        processes[i] = start_process(binary_paths[i], shared_mem_fd, suppress);
    }
    
    char *seller = "./sellers";
    //
    // for (int i = 0; i < game->config.NUM_SELLERS; i++) {
    //     processes_sellers[i] = start_process(seller, 0, false);
    //     game->info.sellers[i].id = i;
    //     game->info.sellers[i].pid = processes_sellers[i];
    // }

    return 0;
}


pid_t start_process(const char *binary, int shared_mem_fd, bool suppress) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        // Convert fd to string

        char fd_str[10];

        if (suppress)
            freopen("/dev/null", "w", stdout);
        snprintf(fd_str, sizeof(fd_str), "%d", shared_mem_fd);
        if (execl(binary, binary, fd_str, NULL)) {

            printf("%s\n", binary);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}


int check_game_conditions(const Game *game) {

    if (game->elapsed_time > game->config.MAX_TIME) {
        return 0;
    }
    if (game->num_frustrated_customers >= game->config.FRUSTRATED_CUSTOMERS) {
        return 0;
    }
    if (game->num_complained_customers >= game->config.COMPLAINED_CUSTOMERS) {
        return 0;
    }
    if (game->num_customers_missing >= game->config.CUSTOMERS_MISSING) {
        return 0;
    }
    if (game->daily_profit > game->config.DAILY_PROFIT) {
        return 0;
    }
    return 1;
}