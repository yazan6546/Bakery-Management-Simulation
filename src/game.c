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


int game_init(Game *game, pid_t *processes, int shared_mem_fd) {

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
        // "./graphics",
        // "./chefs",
        // "./bakers",
        // "./sellers",
        "./supply_chain_manager"
        // "./customer_manager"
    };

    for (int i = 0; i < 1; i++) {
        processes[i] = start_process(binary_paths[i], shared_mem_fd);
    }

    return 0;
}

void game_create(int *shm_fd, Game **shared_game) {
    // Create shared game state using mmap
    *shm_fd = shm_open("/game_shared_mem", O_CREAT | O_RDWR, 0666);
    if ((*shm_fd) == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(*shm_fd, sizeof(Game)) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    *shared_game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (*shared_game == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    fcntl(*shm_fd, F_SETFD, fcntl(*shm_fd, F_GETFD) & ~FD_CLOEXEC);


}

void game_destroy(const int shm_fd, Game *shared_game) {
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
    shm_unlink("/game_shared_mem");

    if (shm_fd > 0) {
        close(shm_fd);
    }
}


pid_t start_process(const char *binary, int shared_mem_fd) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        // Convert fd to string
        char fd_str[10];
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
//
// int check_round_conditions(const Game *game, const Config *config) {
//     if (game->round_time > config->MAX_ROUND_TIME) {
//         return 0;
//     }
//
//     if (game->round_score >= config->WINNING_THRESHOLD || game->round_score <= -config->WINNING_THRESHOLD) {
//         return 0;
//     }
//     return 1;
// }
//
// void go_to_next_round(Game *game) {
//     game->round_num++;
//     game->round_score = 0;
//     game->round_running = 1;
//     game->reset_round_time_flag = 1; // Reset round time
// }
//
//
// void print_with_time1(const Game *game, const char *format, ...) {
//     va_list args;
//     va_start(args, format);
//     printf("@ g:%ds r:%ds: ", game->elapsed_time, game->round_time);
//     vprintf(format, args);
//     va_end(args);
//     fflush(stdout);
// }
