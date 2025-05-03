#include "chef.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "game.h"
#include "shared_mem_utils.h"
#include "semaphores_utils.h"
#include "inventory.h"


Game *game;

int main(int argc, char *argv[]) {

    setup_shared_memory(&game);

    // Setup semaphores
    sem_t* inventory_sem = setup_inventory_semaphore();
    sem_t* ready_products_sem = setup_ready_products_semaphore();

    // Setup signal handler for chef reassignment
    signal(SIGUSR1, SIG_IGN);  // Parent process ignores the signal

    time_t last_check_time = time(NULL);
    
    if (!inventory_sem || !ready_products_sem) {
        perror("Failed to setup semaphores");
        exit(1);
    }

    // Create message queues for each team
    int team_queues[TEAM_COUNT];
    for (int i = 0; i < TEAM_COUNT; i++) {
        team_queues[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        if (team_queues[i] == -1) {
            perror("Failed to create team message queue");
            exit(1);
        }
    }

    // Initialize chef manager
    ChefManager* manager = init_chef_manager(&game->productCatalog,
                                           inventory_sem,
                                           ready_products_sem);

    // Initial distribution of chefs per team
    int chefs_per_team[TEAM_COUNT] = {
        [TEAM_PASTE] = 2,
        [TEAM_BREAD] = 2,
        [TEAM_CAKES] = 2,
        [TEAM_SANDWICHES] = 2,
        [TEAM_SWEETS] = 2,
        [TEAM_SWEET_PATISSERIES] = 2,
        [TEAM_SAVORY_PATISSERIES] = 2
    };

    // Spawn chef workers for each team
    for (int team = 0; team < TEAM_COUNT; team++) {
        for (int i = 0; i < chefs_per_team[team]; i++) {
            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                char mqid_str[16], team_str[8];
                snprintf(mqid_str, sizeof(mqid_str), "%d", team_queues[team]);
                snprintf(team_str, sizeof(team_str), "%d", team);

                execl("./chef_worker", "chef_worker", mqid_str, team_str, NULL);
                perror("execl failed");
                exit(1);
            } else if (pid > 0) {
                // Parent process
                Chef* chef = &manager->chefs[manager->chef_count++];
                chef->id = manager->chef_count;
                chef->team = team;
                chef->pid = pid;
                chef->is_active = 1;
            } else {
                perror("Fork failed");
            }
        }
    }

    while (1) {
        // Process messages from chefs
        process_chef_messages(manager, team_queues, game);

        // Check if it's time to rebalance teams
        time_t current_time = time(NULL);
        if (current_time - last_check_time >= game->config.REALLOCATION_CHECK_INTERVAL) {
            balance_teams(manager, game);
            last_check_time = current_time;
        }

        usleep(100000);  // Small delay to prevent busy waiting
    }

    // Cleanup
    cleanup_inventory_semaphore_resources(inventory_sem);
    cleanup_ready_products_semaphore_resources(ready_products_sem);
    
    return 0;
}