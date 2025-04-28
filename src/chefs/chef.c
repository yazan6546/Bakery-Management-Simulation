#include "chef.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "game.h"
#include <sys/mman.h>

#define NUM_CHEF_TEAMS 6
#define SIMULATION_TIME 30  // seconds

// Global flag for signal handling
volatile sig_atomic_t stop_simulation = 0;

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    stop_simulation = 1;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <fd>\n", argv[0]);
    }

    int fd = atoi(argv[1]);  // 4th argument for mmap
    // Open the file descriptor for reading
    Game *shared_game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_game == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    Config config = shared_game->config;

    srand(time(NULL));

    // Set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Mock ingredients (in real implementation, this will be shared memory)
    BakeryIngredients ingredients = {
        .wheat = 50, .yeast = 20, .butter = 30, .milk = 40,
        .sugar = 50, .salt = 20, .sweet_items = 30,
        .cheese = 25, .salami = 25, .eggs = 30
    };

    // Initialize chef teams
    ChefTeam teams[];
    initialize_chef_teams(teams, NUM_CHEF_TEAMS);

    // Print initial status
    printf("\n=== INITIAL TEAM STATUS ===\n");
    for (int i = 0; i < NUM_CHEF_TEAMS; i++) {
        print_team_status(&teams[i]);
    }

    // Create chef processes
    printf("\n=== STARTING CHEF TEAMS ===\n");
    create_chef_processes(teams, NUM_CHEF_TEAMS, &ingredients);

    // Main simulation loop
    printf("\n=== SIMULATION RUNNING ===\n");
    time_t start_time = time(NULL);
    while (!stop_simulation && difftime(time(NULL), start_time) < config.MAX_TIME) {
        sleep(1); // Main process just waits

        // In a real implementation, you might monitor shared memory here
        // and make decisions about reallocating chefs between teams
    }

    // Clean up
    printf("\n=== SIMULATION ENDING ===\n");
    cleanup_chef_processes(teams, NUM_CHEF_TEAMS);

    // Print final status
    printf("\n=== FINAL TEAM STATUS ===\n");
    for (int i = 0; i < NUM_CHEF_TEAMS; i++) {
        print_team_status(&teams[i]);
    }

    return 0;
}