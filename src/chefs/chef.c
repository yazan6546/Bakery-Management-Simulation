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
#include "team.h"
#include "bakery_message.h"


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

   int msg_queue = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msg_queue == -1) {
        perror("Failed to create message queue");
        exit(1);
    }


    // Initialize chef manager
    ChefManager* manager = init_chef_manager(&game->productCatalog,
                                           inventory_sem,
                                           ready_products_sem);

    int chefs_per_team[TEAM_COUNT] = {0};  // Initialize all to 0
    int remaining_chefs = game->config.NUM_CHEFS;

    // First assign 1 chef to each team
    for (int i = 0; i < TEAM_COUNT; i++) {
        chefs_per_team[i] = 1;
        remaining_chefs--;
    }

    // Randomly distribute remaining chefs
    while (remaining_chefs > 0) {
        int team = rand() % TEAM_COUNT;
        chefs_per_team[team]++;
        remaining_chefs--;
    }

    // Spawn chef workers for each team
    int chef_count = 0;
    for (int team = 0; team < TEAM_COUNT; team++) {
        for (int i = 0; i < chefs_per_team[team]; i++) {
            int id = chef_count++;
            
            // Initialize chef info in parent process
            Chef* chef = &game->info.chefs[id];
            chef->id = id;
            chef->team = team;
            chef->is_active = 1;
            
            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                char mqid_str[16], team_str[8], id_str[8];

                snprintf(mqid_str, sizeof(mqid_str), "%d", msg_queue);
                snprintf(team_str, sizeof(team_str), "%d", team);
                snprintf(id_str, sizeof(id_str), "%d", id);

                execl("./chef_worker", "chef_worker", mqid_str, team_str, id_str, NULL);
                perror("execl failed");
                exit(1);
            } else if (pid > 0) {
                // Store PID in parent process
                game->info.chefs[id].pid = pid;
            } else {
                perror("Fork failed");
            }
        }
    }

    int baker_msg_queue = msgget(CHEF_BAKER_KEY, 0666 | IPC_CREAT);

    while (1) {
        // Process messages from chefs
        process_chef_messages(manager, msg_queue, baker_msg_queue, game);

        // Check if it's time to rebalance teams
        time_t current_time = time(NULL);
        if (current_time - last_check_time >= game->config.REALLOCATION_CHECK_INTERVAL) {
            balance_teams(game);
            last_check_time = current_time;
        }

        usleep(100000);  // Small delay to prevent busy waiting
    }

    // Cleanup
    cleanup_inventory_semaphore_resources(inventory_sem);
    cleanup_ready_products_semaphore_resources(ready_products_sem);
    
    return 0;
}