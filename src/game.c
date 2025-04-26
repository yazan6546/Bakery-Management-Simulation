//
// Created by - on 3/26/2025.
//

#include "game.h"
#include "inventory.h"
#include <message.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "config.h"
#include "common.h"
#include <string.h>


int game_init(Game *game, pid_t *processes) {
    game->elapsed_time = 0;
    game->num_frustrated_customers = 0;
    game->num_complained_customers = 0;
    game->num_customers_missing = 0;

    init_inventory(&game->inventory);

    if (load_config("../config.txt", &game->config) == -1) {
        printf("Config file failed");
        return 1;
    }

    char *binary_paths[] = {
        "./graphics",
        "./chefs",
        "./bakers",
        "./sellers",
        "./supply_chain",
        "./customers"
    };

    for (int i = 0; i < 6; i++) {
        processes[i] = start_process(binary_paths[i], &game->config);
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


pid_t start_process(const char *binary, Config *config) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        char buffer[50];
        serialize_config(config, buffer);
        if (execl(binary, binary, buffer, NULL)) {

            printf("%s\n", binary);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}

//
// Team simulate_round(int pipe_fds_team_A[], int pipe_fds_team_B[], const Config *config, Game *game) {
//     float total_effort_A = 0, total_effort_B = 0;
//     char output_buffer[4096] = "";  // Large buffer for all output
//     char temp_buffer[256];          // Temporary buffer for formatting
//
//     strcat(output_buffer, "\n");
//
//     // Process Team A efforts
//     for (int i = 0; i < config->NUM_PLAYERS/2; i++) {
//         Message message;
//         int index_pipe = game->players_teamA[i].number;
//         ssize_t bytes = read(pipe_fds_team_A[index_pipe], &message, sizeof(Message));
//
//         if (bytes == sizeof(Message) || bytes == 0) {
//             snprintf(temp_buffer, sizeof(temp_buffer),
//                      "Team A - Player %d effort: %.2f\n", game->players_teamA[i].number, message.effort);
//             strcat(output_buffer, temp_buffer);
//             total_effort_A += message.effort;
//             game->players_teamA[i].attributes.energy = message.effort / game->players_teamA[i].position;
//             game->players_teamA[i].state = message.state;
//         }
//         else {
//             perror("read");
//             fflush(stderr);
//             usleep(10000);
//             // Sleep briefly to allow output to be written
//             exit(1);
//         }
//
//     }
//
//     // Process Team B efforts
//     for (int i = 0; i < config->NUM_PLAYERS/2; i++) {
//         Message message;
//         int index_pipe = game->players_teamB[i].number;
//         ssize_t bytes = read(pipe_fds_team_B[index_pipe], &message, sizeof(Message));
//
//         if (bytes == sizeof(Message) || bytes == 0) {
//             snprintf(temp_buffer, sizeof(temp_buffer),
//                      "Team B - Player %d effort: %.2f\n", game->players_teamB[i].number, message.effort);
//             strcat(output_buffer, temp_buffer);
//             total_effort_B += message.effort;
//             game->players_teamB[i].attributes.energy = message.effort / game->players_teamB[i].position;
//             game->players_teamB[i].state = message.state;
//         }
//         else {
//             perror("read");
//             fflush(stderr);
//             usleep(10000);
//             // Sleep briefly to allow output to be written
//             exit(1);
//         }
//     }
//
//     // Calculate round score and add totals
//     game->round_score = total_effort_A - total_effort_B;
//     game->total_effort_A = total_effort_A;
//     game->total_effort_B = total_effort_B;
//     snprintf(temp_buffer, sizeof(temp_buffer),
//              "\nTotal Effort A: %.2f | Total Effort B: %.2f | Score: %.2f\n\n",
//              total_effort_A, total_effort_B, game->round_score);
//     strcat(output_buffer, temp_buffer);
//
//     // Determine winner
//     Team winner = NONE;
//
//     if (game->round_score >= config->WINNING_THRESHOLD) {
//         strcat(output_buffer, "🏆 Team A wins!\n");
//         strcat(output_buffer, "Round score exceeded threshold!\n");
//         game->team_wins_A++;
//         winner = TEAM_A;
//     }
//     else if (game->round_score <= -config->WINNING_THRESHOLD) {
//         strcat(output_buffer, "🏆 Team B wins!\n");
//         strcat(output_buffer, "Round score exceeded threshold!\n");
//         game->team_wins_B++;
//         winner = TEAM_B;
//     }
//     else if (game->round_time > config->MAX_ROUND_TIME) {
//         if (game->round_score > 0) {
//
//             strcat(output_buffer, "🏆 Team A wins!\n");
//             game->team_wins_A++;
//             winner = TEAM_A;
//         }
//         else if (game->round_score < 0) {
//             strcat(output_buffer, "🏆 Team B wins!\n");
//             game->team_wins_B++;
//             winner = TEAM_B;
//         }
//         else {
//             strcat(output_buffer, "It's a draw!\n");
//         }
//
//         strcat(output_buffer, "Round Time is up!\n");
//     }
//     else if (game->elapsed_time > config->MAX_TIME) {
//         if (game->round_score > 0) {
//             strcat(output_buffer, "🏆 Team A wins!\n");
//             game->team_wins_A++;
//             winner = TEAM_A;
//         } else if (game->round_score < 0) {
//             strcat(output_buffer, "🏆 Team B wins!\n");
//             game->team_wins_B++;
//             winner = TEAM_B;
//         }
//         else {
//             strcat(output_buffer, "It's a draw!\n");
//         }
//
//         strcat(output_buffer, "Game time is up!\n");
//     }
//
//     // Print everything at once with timestamp
//     print_with_time1(game, "%s", output_buffer);
//     fflush(stdout);
//
//     return winner;
// }

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
