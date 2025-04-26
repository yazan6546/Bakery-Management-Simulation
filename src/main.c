#include "common.h"
#include <signal.h>
#include "game.h"
#include "config.h"
#include <stdio.h>
#include "assets.h"

// Global pointer to shared game state
Game *shared_game;
pid_t processes[6];
int shm_fd; // Store fd globally for cleanup

void handle_alarm(int signum);
void handle_sigint(int signum); // Renamed from handle_sigkill to match actual signal
void cleanup_resources(void);   // New function for atexit

void handle_alarm(int signum) {

    shared_game->elapsed_time++;

    alarm(1);
}

void handle_sigint(int signum) {
    exit(0); // Let atexit handle cleanup
}

// Function for cleaning up resources, registered with atexit()
void cleanup_resources(void) {
    printf("Cleaning up resources...\n");
    fflush(stdout);

    for (int i = 0; i < 6; i++) {
        kill(processes[i], SIGKILL);
    }

    game_destroy(shm_fd, shared_game);

    printf("Cleanup complete\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {

    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);

    // Register cleanup function with atexit
    atexit(cleanup_resources);
    game_create(&shm_fd, &shared_game);
    game_init(shared_game, processes);


    signal(SIGALRM, handle_alarm);
    signal(SIGINT, handle_sigint); // Renamed to match actual signal
    alarm(1);  // Start the timer


    while (check_game_conditions(shared_game)) {



    }

}