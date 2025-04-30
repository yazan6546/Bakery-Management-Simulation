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

#include "inventory.h"
#include "config.h"
#include "game.h"

// Message structure for restock requests
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int urgency; // 0-10 scale, 10 being most urgent
} RestockRequest;

// Message structure for restock confirmations
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int success; // 1 if successful, 0 if failed
} RestockConfirmation;

// Global game reference
Game *game;

// Function to process a restock request
void process_restock_request(RestockRequest *request, int response_queue_id) {
    printf("[Supply Chain] Processing restock request for ingredient %d, quantity %d, urgency %d\n", 
           request->ingredient, request->quantity, request->urgency);
    
    // Simulate delivery time based on urgency (more urgent = faster delivery)
    int delivery_time = 10 - request->urgency;
    if (delivery_time < 1) delivery_time = 1;
    
    printf("[Supply Chain] Estimated delivery time: %d seconds\n", delivery_time);
    
    // Simulate the delivery process
    sleep(delivery_time);
    
    // Create and prepare confirmation message
    RestockConfirmation confirmation;
    confirmation.mtype = request->mtype; // Reply to the same type
    confirmation.ingredient = request->ingredient;
    confirmation.quantity = request->quantity;
    confirmation.success = 1; // Assume success for now
    
    // Add ingredients to inventory
    lock_inventory();
    add_ingredient(&game->inventory, request->ingredient, request->quantity);
    unlock_inventory();
    
    // Send confirmation back to chef
    if (msgsnd(response_queue_id, &confirmation, sizeof(RestockConfirmation) - sizeof(long), 0) == -1) {
        perror("[Supply Chain] Failed to send confirmation");
    } else {
        printf("[Supply Chain] Restocked %d units of ingredient %d, confirmation sent\n", 
               request->quantity, request->ingredient);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <shared_memory_fd> <request_queue_id> <response_queue_id>\n", argv[0]);
        return 1;
    }
    
    srand(time(NULL));
    
    // Parse command line arguments
    int shm_fd = atoi(argv[1]);
    int request_queue_id = atoi(argv[2]);
    int response_queue_id = atoi(argv[3]);
    
    // Map shared memory
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    
    printf("[Supply Chain] Supply chain process started (PID: %d)\n", getpid());
    printf("[Supply Chain] Monitoring request queue %d, responding on queue %d\n", 
           request_queue_id, response_queue_id);
    
    // Setup inventory semaphores if needed
    if (setup_inventory_semaphore() == -1) {
        perror("Failed to set up inventory semaphore");
        exit(1);
    }
    
    // Main loop to process restock requests
    while (1) {
        RestockRequest request;
        
        // Wait for a restock request
        if (msgrcv(request_queue_id, &request, sizeof(RestockRequest) - sizeof(long), 0, 0) == -1) {
            perror("[Supply Chain] Error receiving message");
            continue;
        }
        
        // Process the request
        process_restock_request(&request, response_queue_id);
    }
    
    return 0;
}