#include "chef.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

// Convert team type to string
const char* get_team_type_name(TeamType type) {
    static const char* names[] = {
        "Paste Preparation",
        "Cake Preparation",
        "Sandwich Preparation",
        "Sweets Preparation",
        "Sweet Patisserie",
        "Savory Patisserie"
    };
    return names[type];
}

// Print team status
void print_team_status(const ChefTeam *team) {
    printf("[Team %d: %s] PID: %d | Status: %s | Produced: %d\n",
           team->team_id,
           get_team_type_name(team->type),
           team->pid,
           team->is_active ? "Active" : "Inactive",
           team->items_produced);
}

void serialize_chef_team(ChefTeam *team, char *buffer) {
    sprintf(buffer, "%d %d %d %d %d %d %d",
            team->team_id,
            team->pid,
            team->type,
            team->team_size,
            team->is_active,
            team->depends_on,
            team->items_produced);
}

// Initialize chef teams based on configuration
void initialize_chef_teams(ChefTeam *teams, int num_teams) {
    for (int i = 0; i < num_teams; i++) {
        teams[i].team_id = i;
        teams[i].pid = -1;
        teams[i].is_active = true;
        teams[i].items_produced = 0;

        // Assign team types and dependencies
        switch (i % NUM_TEAM_TYPES) {
            case TEAM_PASTE:
                teams[i].type = TEAM_PASTE;
                teams[i].depends_on = -1;
                teams[i].items_required = 0;
                teams[i].team_size = 2;
                break;
            case TEAM_CAKES:
                teams[i].type = TEAM_CAKES;
                teams[i].depends_on = -1;
                teams[i].items_required = 0;
                teams[i].team_size = 3;
                break;
            case TEAM_SANDWICHES:
                teams[i].type = TEAM_SANDWICHES;
                teams[i].depends_on = -1;
                teams[i].items_required = 1;
                teams[i].team_size = 2;
                break;
            case TEAM_SWEETS:
                teams[i].type = TEAM_SWEETS;
                teams[i].depends_on = -1;
                teams[i].items_required = 0;
                teams[i].team_size = 2;
                break;
            case TEAM_SWEET_PATISSERIE:
                teams[i].type = TEAM_SWEET_PATISSERIE;
                teams[i].depends_on = TEAM_PASTE;
                teams[i].items_required = 1;
                teams[i].team_size = 2;
                break;
            case TEAM_SAVORY_PATISSERIE:
                teams[i].type = TEAM_SAVORY_PATISSERIE;
                teams[i].depends_on = TEAM_PASTE;
                teams[i].items_required = 1;
                teams[i].team_size = 2;
                break;
        }
    }
}

// Function to check if ingredients are available
bool check_ingredients(TeamType type, const BakeryIngredients *ingredients) {
    switch (type) {
        case TEAM_PASTE:
            return (ingredients->wheat >= 2 && ingredients->yeast >= 1);
        case TEAM_CAKES:
            return (ingredients->butter >= 1 && ingredients->sugar >= 2 &&
                    ingredients->milk >= 1 && ingredients->eggs >= 2);
        case TEAM_SANDWICHES:
            return (ingredients->cheese >= 1 && ingredients->salami >= 1);
        case TEAM_SWEETS:
            return (ingredients->sugar >= 3 && ingredients->sweet_items >= 2);
        case TEAM_SWEET_PATISSERIE:
            return (ingredients->butter >= 1 && ingredients->sugar >= 1);
        case TEAM_SAVORY_PATISSERIE:
            return (ingredients->butter >= 1 && ingredients->salt >= 1);
        default:
            return false;
    }
}

// Function to simulate production work
void perform_team_work(TeamType type) {
    // Different teams take different amounts of time
    int delay = 1;
    switch (type) {
        case TEAM_PASTE: delay = 2; break;
        case TEAM_CAKES: delay = 3; break;
        case TEAM_SANDWICHES: delay = 1; break;
        case TEAM_SWEETS: delay = 2; break;
        case TEAM_SWEET_PATISSERIE:
        case TEAM_SAVORY_PATISSERIE: delay = 2; break;
    }
    sleep(delay);
}

// Function that runs in each team process
void chef_team_work(ChefTeam *team, BakeryIngredients *ingredients) {
    printf("CHEF TEAM %d (%s) STARTED (PID: %d)\n",
           team->team_id, get_team_type_name(team->type), getpid());

    while (team->is_active) {
        // Check if we have needed ingredients
        if (!check_ingredients(team->type, ingredients)) {
            printf("Team %d (PID: %d) waiting for ingredients...\n",
                   team->team_id, getpid());
            sleep(2);
            continue;
        }

        // Do the work
        perform_team_work(team->type);
        team->items_produced++;

        printf("Team %d (PID: %d) produced item #%d\n",
               team->team_id, getpid(), team->items_produced);
    }

    printf("Team %d (PID: %d) EXITING\n", team->team_id, getpid());
    exit(EXIT_SUCCESS);
}

void create_chef_processes(const char *binary,ChefTeam *teams, int num_teams, BakeryIngredients *ingredients, Config *config) {
    for (int i = 0; i < num_teams; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            char config_buffer[512];
            serialize_config(config, config_buffer);
            char team_buffer[512];
            serialize_chef_team(&teams[i], team_buffer);
            if (execl(binary, binary,team_buffer,config_buffer,NULL)) {
                perror("execl failed");
                exit(EXIT_FAILURE);
            }
        }
        else {
            teams[i].pid = pid;
        }

    }
}

// Clean up all chef processes
void cleanup_chef_processes(ChefTeam *teams, int num_teams) {
    printf("\nCleaning up chef processes...\n");

    // First try to terminate gracefully
    for (int i = 0; i < num_teams; i++) {
        if (teams[i].pid > 0) {
            if (kill(teams[i].pid, SIGTERM) == -1) {
                perror("Failed to send SIGTERM to chef team");
            }
        }
    }

    // Wait for processes to terminate
    sleep(2);

    // Force kill any remaining processes
    for (int i = 0; i < num_teams; i++) {
        if (teams[i].pid > 0) {
            if (waitpid(teams[i].pid, NULL, WNOHANG) == 0) {
                if (kill(teams[i].pid, SIGKILL) == -1) {
                    perror("Failed to send SIGKILL to chef team");
                }
                printf("Force killed team %d (PID: %d)\n", i, teams[i].pid);
            }
        }
    }

    // Wait for all processes
    for (int i = 0; i < num_teams; i++) {
        if (teams[i].pid > 0) {
            waitpid(teams[i].pid, NULL, 0);
            printf("Team %d (PID: %d) has terminated\n", i, teams[i].pid);
        }
    }
}