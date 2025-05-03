//
// Created by yazan on 4/26/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "bakery_message.h"
#include "inventory.h"
#include "config.h"
#include "game.h"
#include "supply_chain.h"
#include "chef.h"  // Include chef.h to access RestockRequest type

// Global game reference
Game *game;

// Function to process a restock request
void process_restock_request(SupplyChainState *state, SupplyChainRequest *request, Inventory *inventory) {
    printf("[Supply Chain %d] Processing restock request for ingredient %d, quantity %d, urgency %d\n", 
           state->id, request->ingredient, request->quantity, request->urgency);
    
    // Simulate delivery time based on urgency (more urgent = faster delivery)
    int delivery_time = 10 - request->urgency;
    if (delivery_time < 1) delivery_time = 1;
    
    printf("[Supply Chain %d] Estimated delivery time: %d seconds\n", state->id, delivery_time);
    
    // Simulate the delivery process
    sleep(delivery_time);
    
    // Create and prepare confirmation message
    SupplyChainConfirmation confirmation;
    confirmation.mtype = request->mtype; // Reply to the same type
    confirmation.ingredient = request->ingredient;
    confirmation.quantity = request->quantity;
    confirmation.success = 1; // Assume success for now
    
    // Add ingredients to inventory
    add_ingredient(inventory, request->ingredient, request->quantity, state->inventory_sem);
    
    // Send confirmation back to chef
    if (msgsnd(state->response_queue_id, &confirmation, sizeof(SupplyChainConfirmation) - sizeof(long), 0) == -1) {
        perror("[Supply Chain] Failed to send confirmation");
    } else {
        printf("[Supply Chain %d] Restocked %d units of ingredient %d, confirmation sent\n", 
               state->id, request->quantity, request->ingredient);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <shared_memory_fd> <request_queue_id> <response_queue_id> [supply_chain_id]\n", argv[0]);
        return 1;
    }
    
    srand(time(NULL) ^ getpid());
    
    // Parse command line arguments
    int shm_fd = atoi(argv[1]);
    int request_queue_id = atoi(argv[2]);
    int response_queue_id = atoi(argv[3]);
    int supply_chain_id = (argc > 4) ? atoi(argv[4]) : (getpid() % 100);
    
    // Map shared memory
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    
    printf("[Supply Chain %d] Supply chain process started (PID: %d)\n", supply_chain_id, getpid());
    printf("[Supply Chain %d] Monitoring request queue %d, responding on queue %d\n", 
           supply_chain_id, request_queue_id, response_queue_id);
    
    // Setup inventory semaphores
    sem_t* inventory_sem = setup_inventory_semaphore();
    if (inventory_sem == NULL) {
        perror("Failed to set up inventory semaphore");
        exit(1);
    }
    
    // Initialize supply chain state
    SupplyChainState state;
    state.id = supply_chain_id;
    state.request_queue_id = request_queue_id;
    state.response_queue_id = response_queue_id;
    state.inventory_sem = inventory_sem;
    
    // Main loop to process restock requests
    while (1) {
        // Need to handle both the RestockRequest from chefs and convert it to our SupplyChainRequest format
        RestockRequest chef_request;
        
        // Wait for a restock request
        if (msgrcv(request_queue_id, &chef_request, sizeof(RestockRequest) - sizeof(long), 0, 0) == -1) {
            perror("[Supply Chain] Error receiving message");
            continue;
        }
        
        // Convert the chef request to our format
        SupplyChainRequest supply_request;
        supply_request.mtype = chef_request.mtype;
        supply_request.ingredient = chef_request.ingredient;
        supply_request.quantity = chef_request.quantity;
        supply_request.urgency = chef_request.urgency;
        
        // Process the request
        process_restock_request(&state, &supply_request, &game->inventory);
    }
    
    // Cleanup (this will never execute in the current implementation)
    if (inventory_sem) {
        sem_close(inventory_sem);
    }
    
    return 0;
}