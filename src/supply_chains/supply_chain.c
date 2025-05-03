#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include <math.h>

#include "game.h"
#include "supply_chain.h"
#include "semaphores_utils.h"
#include "shared_mem_utils.h"
#include "random.h"
#include "products.h"

// Define message queue key
#define SUPPLY_CHAIN_MSG_KEY 0x1234

// Global variables
Game* shared_game = NULL;
sem_t* inventory_sem = NULL;
int supply_chain_id = -1;
int msg_queue_id = -1;

// Signal handler for cleanup
void handle_signal(int sig) {
    printf("Supply Chain %d: Shutting down...\n", supply_chain_id);
    
    // Cleanup resources
    if (shared_game != NULL) {
        munmap(shared_game, sizeof(Game));
    }
    
    exit(EXIT_SUCCESS);
}

// Function to generate random delay between deliveries
int get_random_delay() {
    return (rand() % 5) + 3; // 3 to 7 seconds
}

// Function to generate random quantity of ingredients
int get_random_quantity() {
    return (rand() % 10) + 5; // 5 to 14 units
}



// Function to update inventory based on supply chain type
void update_inventory() {
    
    SupplyChainMessage* msg;
    msg = malloc(sizeof(SupplyChainMessage) + sizeof(Ingredient) * shared_game->config.INGREDIENTS_TO_ORDER);
    if (msg == NULL) {
        perror("Failed to allocate memory for message");
        return;
    }
    int result = msgrcv(msg_queue_id, msg, sizeof(SupplyChainMessage) + sizeof(Ingredient) * shared_game->config.INGREDIENTS_TO_ORDER  - sizeof(long), getpid(), IPC_NOWAIT);

    if(result == -1) {
        perror("Failed to receive message from supply chain");
        return;
    }


    // Simulate delivery time
    int time = get_random_delay();
    printf("Supply Chain %d: delivering after %d seconds\n", getpid(), time);
    sleep(time);
    printf("Supply Chain %d: putting in inventory\n", getpid());
    
    
    // Lock inventory for update
    lock_inventory(inventory_sem);

    printf("Supply Chain %d: Accessed inventory:\n", getpid());
    
    // Update inventory in shared memory
    for (int i = 0; i < shared_game->config.INGREDIENTS_TO_ORDER; i++)
    {
        // Update the inventory in shared memory 

        int type = msg->ingredients[i].type;
        shared_game->inventory.quantities[type] = 
        fmin(shared_game->inventory.quantities[type] + msg->ingredients[i].quantity,
             (float)shared_game->inventory.max_capacity);
           
        printf("Supply Chain %d: Updated inventory for ingredient %d: %.1f\n", 
               getpid(), i, shared_game->inventory.quantities[type]);
    }

    unlock_inventory(inventory_sem);

    print_inventory(&shared_game->inventory);

    free(msg);

    fflush(stdout);
    
}

int main(int argc, char *argv[]) {
  
    
    // Setup shared memory
    setup_shared_memory(&shared_game);
    if (shared_game == NULL) {
        fprintf(stderr, "Supply Chain %d: Failed to setup shared memory\n", supply_chain_id);
        return EXIT_FAILURE;
    }
    
    // Setup semaphores
    inventory_sem = setup_inventory_semaphore();
    if (inventory_sem == NULL) {
        fprintf(stderr, "Supply Chain %d: Failed to setup inventory semaphore\n", supply_chain_id);
        return EXIT_FAILURE;
    }
    
    // Create or get message queue
    msg_queue_id = msgget(SUPPLY_CHAIN_MSG_KEY, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("Failed to create/get message queue");
        return EXIT_FAILURE;
    }
    
    
    // Main loop
    while (1) {
        // Random delay between deliveries
        
        
        // Update inventory with new supplies
        update_inventory();
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}