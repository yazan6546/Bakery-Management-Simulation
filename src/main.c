#include "common.h"
#include <signal.h>
#include "game.h"
#include "config.h"
#include <stdio.h>

// Global pointer to shared game state
Game *shared_game;
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

    game_destroy(shm_fd, shared_game);
    //
    // kill(pid_graphics, SIGKILL);
    // kill(pid_referee, SIGKILL);

    printf("Cleanup complete\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {

    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);

    // Register cleanup function with atexit
    atexit(cleanup_resources);
    game_create()

    signal(SIGALRM, handle_alarm);
    signal(SIGINT, handle_sigint); // Renamed to match actual signal
    alarm(1);  // Start the timer


    wait(&pid_graphics);

}