// created by Ghazi on 5/2/2025
// we need a game struct to spawn supply chains
// we need a semaphore to manage adding to the inventory
// each supply chain will have its own semaphore and will add to the inventory after a random timer

#include "game.h"
#include "semaphores_utils.h"
#include "shared_mem_utils.h"
#include "signal.h"
#include "random.h"

Game* shared_game = NULL;

void handle_sigint(int sig) {
    // Cleanup code here
    exit(0);
}

void cleanup_supply_chain_resources() {
    if (shared_game != NULL) {
        munmap(shared_game, sizeof(Game));
        shm_unlink("/game_shared_mem");
    }
}

void fork_supply_chain_process() {

    for(int i = 0; i < shared_game->config.NUM_SUPPLY_CHAIN; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char supply_chain_id[10];
            snprintf(supply_chain_id, sizeof(supply_chain_id), "%d", i);
            execl("./supply_chain", "./supply_chain", supply_chain_id, NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("Failed to fork supply chain process");
        }
    }
}



int main(int argc, char *argv[]) {
    // Register signal handlers for cleanup
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    // Setup shared memory and semaphores
    setup_shared_memory(&shared_game);
    sem_t* inventory_sem = setup_inventory_semaphore();
    sem_t* ready_products_sem = setup_ready_products_semaphore();

    // Register cleanup handler
    atexit(cleanup_supply_chain_resources);
    // Initialize random number generator
    init_random();
    

    // Check if shared memory was created successfully
    if (shared_game == NULL) {
        fprintf(stderr, "Failed to setup shared memory\n");
        return 1;
    }

    // Check if semaphores were created successfully
    if (inventory_sem == NULL || ready_products_sem == NULL) {
        fprintf(stderr, "Failed to setup semaphores\n");
        return 1;
    }

    // Main loop for supply chain manager
    while(1) {}

    return 0;

}
