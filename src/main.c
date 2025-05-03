#include <signal.h>
#include "game.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assets.h"

// Global pointer to shared game state
Game *shared_game;
pid_t processes[6];
pid_t *processes_sellers;
int shm_fd; // Store fd globally for cleanup

void handle_alarm(int signum);
void cleanup_resources();
void handle_kill(int signum);


int main(int argc, char *argv[]) {

    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);


    // execlp("pwd", "pwd", NULL);
    // Register cleanup function with atexit
    atexit(cleanup_resources);
    game_create(&shm_fd, &shared_game);

    processes_sellers = malloc(shared_game->config.NUM_SELLERS * sizeof(pid_t));

    signal(SIGALRM, handle_alarm);
    signal(SIGINT, handle_kill); // Renamed to match actual signal


    if (load_config(CONFIG_PATH, &shared_game->config) == -1) {
        printf("Config file failed");
        return 1;
    }

    if (load_product_catalog(CONFIG_PATH_JSON, &shared_game->productCatalog) == -1) {
        printf("Product catalog file failed");
        return 1;
    }

    game_init(shared_game, processes, processes_sellers, shm_fd);

    alarm(1);  // Start the timer


    while (check_game_conditions(shared_game)) {



    }

}

void handle_alarm(int signum) {

    shared_game->elapsed_time++;

    alarm(1);
}


// Function for cleaning up resources, registered with atexit()
void cleanup_resources() {
    printf("Cleaning up resources...\n");
    fflush(stdout);

    for (int i = 0; i < 6; i++) {
        kill(processes[i], SIGINT);
    }

    for (int i = 0; i<shared_game->config.NUM_SELLERS; i++) {
        kill(processes_sellers[i], SIGINT);
    }

    printf("Cleanup complete\n");
    fflush(stdout);
}

void handle_kill(int signum) {
    exit(0);
}