// created by Ghazi on 5/2/2025
// we need a game struct to spawn supply chains
// we need a semaphore to manage adding to the inventory
// each supply chain will have its own semaphore and will add to the inventory after a random timer

#include "game.h"
#include "semaphores_utils.h"
#include "shared_mem_utils.h"
#include "supply_chain.h"
#include "signal.h"
#include "random.h"
#include "products.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>

// Define message queue key (must match the one in supply_chain.c)
#define SUPPLY_CHAIN_MSG_KEY 0x1234
#define INGREDIENTS_TO_ORDER 3

// Global variables
Game* shared_game = NULL;
int msg_queue_id = -1;
pid_t* supply_chain_pids = NULL;

// Function prototypes
void cleanup_supply_chain_resources(void);

void handle_sigint(int sig) {
    exit(0);
}


void cleanup_supply_chain_resources() {
    printf("Supply Chain Manager: Shutting down...\n");
    
    // Terminate all supply chain processes
    if (supply_chain_pids != NULL) {
        for (int i = 0; i < shared_game->config.NUM_SUPPLY_CHAIN; i++) {
            if (supply_chain_pids[i] > 0) {
                kill(supply_chain_pids[i], SIGTERM);
            }
        }
        free(supply_chain_pids);
    }
    
    // Remove message queue
    if (msg_queue_id != -1) {
        msgctl(msg_queue_id, IPC_RMID, NULL);
    }
    
        if (shared_game != NULL) {
        munmap(shared_game, sizeof(Game));
        shm_unlink("/game_shared_mem");
    }
}



// Function to process messages from supply chains
void process_supply_chain_messages() {
    SupplyChainMessage msg;
    int pid_index = rand() % shared_game->config.NUM_SUPPLY_CHAIN;

    printf("Supply Chain Manager: Processing messages from supply chain %d\n", pid_index);

    for(int i = 0; i < INGREDIENTS_TO_ORDER; i++) {
        int ingredient_type = rand() % NUM_INGREDIENTS;
        // calculate percentage of this ingredient
        int percentage = shared_game->inventory.quantities[ingredient_type] * 100.0 / shared_game->inventory.max_capacity;

        if(percentage < 20) {
            // Send message to supply chain
            
            msg.mtype = supply_chain_pids[pid_index];
            msg.ingredients[i].type = ingredient_type;
            msg.ingredients[i].quantity = rand() % (shared_game->inventory.max_capacity - shared_game->inventory.quantities[ingredient_type]) + 1; // Random quantity to order

            printf("Supply Chain Manager: Ordering %d of %s\n", 
                   msg.ingredients[i].quantity, get_ingredient_name(ingredient_type));
            
        
        }
    }

    if (msgsnd(msg_queue_id, &msg, sizeof(SupplyChainMessage) - sizeof(long), IPC_NOWAIT) == -1) {
        perror("Failed to send message to supply chain");
    } else {
        printf("Supply Chain Manager: Sent order to supply chain %d", supply_chain_pids[pid_index]);
    }

    fflush(stdout);  

}

void fork_supply_chain_process() {
    // Allocate memory for supply chain PIDs
    supply_chain_pids = malloc(sizeof(pid_t) * shared_game->config.NUM_SUPPLY_CHAIN);
    if (supply_chain_pids == NULL) {
        perror("Failed to allocate memory for supply chain PIDs");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < shared_game->config.NUM_SUPPLY_CHAIN; i++) {
        supply_chain_pids[i] = fork();
        
        if (supply_chain_pids[i] == 0) {
            // Child process
            char supply_chain_id[10];
            snprintf(supply_chain_id, sizeof(supply_chain_id), "%d", i);
            execl("./supply_chain", "./supply_chain", supply_chain_id, NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        } else if (supply_chain_pids[i] < 0) {
            perror("Failed to fork supply chain process");
        } else {
            const char* ingredient_name = get_ingredient_name(i);
            printf("Supply Chain Manager: Started supply chain %d (%s) with PID %d\n", 
                   i, ingredient_name, supply_chain_pids[i]);
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
    
    // Create message queue
    msg_queue_id = msgget(SUPPLY_CHAIN_MSG_KEY, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("Failed to create message queue");
        return 1;
    }
    
    printf("Supply Chain Manager: Created message queue with ID %d\n", msg_queue_id);
    
    // Fork supply chain processes
    fork_supply_chain_process();
    
    printf("Supply Chain Manager: Started %d supply chains\n", 
           shared_game->config.NUM_SUPPLY_CHAIN);

    // Main loop for supply chain manager
    while(1) {
        // Process messages from supply chains
        process_supply_chain_messages();
        
        sleep(3); // Sleep for a while before processing again
    }

    return 0;
}
