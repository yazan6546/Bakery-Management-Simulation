#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>

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

// Function to send a message to supply chain manager
void send_supply_update(IngredientType ingredient_type, float quantity) {
    SupplyChainMessage msg;
    
    // Set message type
    msg.mtype = 1; // Message type 1 for supply updates
    
    // Initialize all ingredients to 0
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        msg.ingredients[i].type = i;
        msg.ingredients[i].quantity = 0.0;
    }
    
    // Set only the specific ingredient that was updated
    msg.ingredients[ingredient_type].type = ingredient_type;
    msg.ingredients[ingredient_type].quantity = quantity;
    
    // Send message to queue
    if (msgsnd(msg_queue_id, &msg, sizeof(SupplyChainMessage) - sizeof(long), 0) == -1) {
        perror("Failed to send message");
    } else {
        printf("Supply Chain %d: Sent supply update message (Ingredient %d, Qty %.1f)\n", 
               supply_chain_id, ingredient_type, quantity);
    }
}

// Function to update inventory based on supply chain type
void update_inventory() {
    int quantity = get_random_quantity();
    IngredientType ingredient_type = supply_chain_id;
    
    // Lock inventory for update
    sem_wait(inventory_sem);
    
    // Update inventory for the specific ingredient
    shared_game->inventory.quantities[ingredient_type] += quantity;
    printf("Supply Chain %d: Added %d units of ingredient %d, new total: %d\n", 
           supply_chain_id, quantity, ingredient_type, 
           shared_game->inventory.quantities[ingredient_type]);
    
    // Release inventory
    sem_post(inventory_sem);
    
    // Send update to manager
    send_supply_update(ingredient_type, (float)quantity);
}

int main(int argc, char *argv[]) {
    // Check arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <supply_chain_id>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Get supply chain ID from command line
    supply_chain_id = atoi(argv[1]);
    
    // Verify supply chain ID is valid
    if (supply_chain_id < 0 || supply_chain_id >= NUM_INGREDIENTS) {
        fprintf(stderr, "Error: Supply chain ID must be between 0 and %d\n", NUM_INGREDIENTS - 1);
        return EXIT_FAILURE;
    }
    
    // Register signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Seed random number generator
    srand(time(NULL) ^ getpid());
    
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
    
    printf("Supply Chain %d: Started successfully (managing ingredient type %d)\n", 
           supply_chain_id, supply_chain_id);
    
    // Main loop
    while (1) {
        // Random delay between deliveries
        int delay = get_random_delay();
        printf("Supply Chain %d: Next delivery in %d seconds\n", supply_chain_id, delay);
        sleep(delay);
        
        // Update inventory with new supplies
        update_inventory();
    }
    
    return EXIT_SUCCESS;
}