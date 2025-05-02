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

// Global variables
Game* shared_game = NULL;
int msg_queue_id = -1;
pid_t* supply_chain_pids = NULL;

// Function prototypes
void cleanup_supply_chain_resources(void);

// Function to translate ingredient type to string
const char* get_ingredient_name(int ingredient_type) {
    static const char* ingredient_names[] = {
        "Wheat", "Yeast", "Butter", "Milk", "Sugar", "Salt", "Sweet Items",
        "Cheese", "Salami", "Paste Ingredients", "Chocolate", "Flour", "Vanilla",
        "Custard", "Eggs", "Vegetables", "Bread Ingredients", "Cream", "Fruits"
    };
    
    if (ingredient_type >= 0 && ingredient_type < NUM_INGREDIENTS) {
        return ingredient_names[ingredient_type];
    } else {
        return "Unknown";
    }
}

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
    
    // Try to receive message without blocking
    int result = msgrcv(msg_queue_id, &msg, sizeof(SupplyChainMessage) - sizeof(long), 
                         0, IPC_NOWAIT);
    
    if (result >= 0) {
        // Message received, check ingredients array for updates
        for (int i = 0; i < NUM_INGREDIENTS; i++) {
            if (msg.ingredients[i].quantity > 0) {
                const char* ingredient_name = get_ingredient_name(i);
                // Found updated ingredient
                printf("Supply Chain Manager: Received update for ingredient %s: Added %.1f units\n", 
                       ingredient_name, msg.ingredients[i].quantity);
                
                // Check if inventory is low for this ingredient
                sem_t* inventory_sem = setup_inventory_semaphore();
                if (inventory_sem != NULL) {
                    sem_wait(inventory_sem);
                    int current_level = shared_game->inventory.quantities[i];
                    int max_level = shared_game->inventory.max_capacity;
                    float percentage = (float)current_level / max_level * 100;
                    
                    if (percentage < 20) {
                        printf("Supply Chain Manager: WARNING - %s stock is low (%.1f%%)\n",
                               ingredient_name, percentage);
                    } else if (percentage > 80) {
                        printf("Supply Chain Manager: NOTICE - %s stock is high (%.1f%%)\n",
                               ingredient_name, percentage);
                    }
                    
                    sem_post(inventory_sem);
                }
            }
        }
    } else if (errno != ENOMSG) {
        // Error other than "no message"
        perror("Error receiving message");
    }
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
        
        
        
        // Small delay to prevent busy waiting
        usleep(100000); // 100ms
    }

    return 0;
}
