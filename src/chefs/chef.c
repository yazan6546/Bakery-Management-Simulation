//
// Created by yazan on 4/26/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "inventory.h"
#include "config.h"
#include "game.h"
#include "chef.h"

// Message structures for supply chain communication
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int urgency; // 0-10 scale, 10 being most urgent
} RestockRequest;

typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int success; // 1 if successful, 0 if failed
} RestockConfirmation;

// Chef state
typedef struct {
    int id;
    int request_queue_id;
    int response_queue_id;
    int is_waiting_for_ingredients;
    IngredientType waiting_for;
    int waiting_quantity;
} ChefState;

// Shared game reference
Game *game;

// Function to check if ingredients are low and need to be restocked
void check_and_request_ingredients(ChefState *chef) {
    // Lock inventory to check
    lock_inventory();
    
    // Check each ingredient
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        // If ingredient is below threshold, request restock
        if (game->inventory.quantities[i] < 10 && !chef->is_waiting_for_ingredients) {
            int quantity_needed = 50 - game->inventory.quantities[i]; // Restock to 50
            if (quantity_needed <= 0) continue;
            
            printf("[Chef %d] Ingredient %d is low (%d). Requesting restock.\n",
                   chef->id, i, game->inventory.quantities[i]);
            
            // Create restock request
            RestockRequest request;
            request.mtype = 1; // Default message type
            request.ingredient = i;
            request.quantity = quantity_needed;
            
            // Calculate urgency based on how low we are
            request.urgency = 10 - (game->inventory.quantities[i] / 5);
            if (request.urgency < 1) request.urgency = 1;
            if (request.urgency > 10) request.urgency = 10;
            
            // Send request to supply chain
            if (msgsnd(chef->request_queue_id, &request, sizeof(RestockRequest) - sizeof(long), 0) == -1) {
                perror("[Chef] Failed to send restock request");
            } else {
                printf("[Chef %d] Sent restock request for %d units of ingredient %d (urgency: %d)\n",
                       chef->id, quantity_needed, i, request.urgency);
                
                // Update chef state
                chef->is_waiting_for_ingredients = 1;
                chef->waiting_for = i;
                chef->waiting_quantity = quantity_needed;
            }
            
            break; // Only request one ingredient at a time
        }
    }
    
    unlock_inventory();
}

// Function to check for supply chain confirmations
void check_for_confirmations(ChefState *chef) {
    if (!chef->is_waiting_for_ingredients) {
        return;
    }
    
    // Check for confirmation message (non-blocking)
    RestockConfirmation confirmation;
    if (msgrcv(chef->response_queue_id, &confirmation, sizeof(RestockConfirmation) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        printf("[Chef %d] Restock confirmation received for %d units of ingredient %d\n",
               chef->id, confirmation.quantity, confirmation.ingredient);
        
        // Reset waiting state
        chef->is_waiting_for_ingredients = 0;
    }
}

// Function to prepare recipes (simplified for now)
void prepare_recipes(ChefState *chef) {
    // If waiting for ingredients, don't try to cook
    if (chef->is_waiting_for_ingredients) {
        return;
    }
    
    // Simple cooking logic - just check if we have enough of each ingredient
    int ingredients_needed[NUM_INGREDIENTS] = {0};
    int recipe_choice = rand() % 3;  // Randomly choose between 3 recipe types
    
    // Set up ingredient requirements based on recipe
    switch (recipe_choice) {
        case 0:  // Bread recipe
            ingredients_needed[WHEAT] = 5;
            ingredients_needed[YEAST] = 2;
            ingredients_needed[SALT] = 1;
            break;
        
        case 1:  // Cake recipe
            ingredients_needed[WHEAT] = 3;
            ingredients_needed[SUGAR] = 4;
            ingredients_needed[BUTTER] = 2;
            ingredients_needed[MILK] = 1;
            break;
            
        case 2:  // Sandwich recipe
            ingredients_needed[WHEAT] = 2;
            ingredients_needed[CHEESE] = 2;
            ingredients_needed[SALAMI] = 2;
            break;
    }
    
    // Check if we have enough ingredients
    if (check_ingredients(&game->inventory, ingredients_needed)) {
        // Use ingredients
        use_ingredients(&game->inventory, ingredients_needed);
        
        // Add to ready products based on recipe type
        lock_ready_products();
        switch (recipe_choice) {
            case 0:
                add_ready_product(NULL, BREAD, 1);
                printf("[Chef %d] Prepared bread\n", chef->id);
                break;
                
            case 1:
                add_ready_product(NULL, CAKE, 1);
                printf("[Chef %d] Prepared cake\n", chef->id);
                break;
                
            case 2:
                add_ready_product(NULL, SANDWICH, 1);
                printf("[Chef %d] Prepared sandwich\n", chef->id);
                break;
        }
        unlock_ready_products();
        
        // Cook time
        sleep(2);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <shared_memory_fd> <request_queue_id> <response_queue_id>\n", argv[0]);
        return 1;
    }
    
    srand(time(NULL) ^ getpid());  // Seed random number generator
    
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
    
    // Setup inventory semaphores
    if (setup_inventory_semaphore() == -1) {
        perror("Failed to setup inventory semaphore");
        exit(1);
    }
    
    // Setup ready products semaphore
    if (setup_ready_products_semaphore() == -1) {
        perror("Failed to setup ready products semaphore");
        exit(1);
    }
    
    // Initialize chef state
    ChefState chef;
    chef.id = getpid() % 100;  // Simple ID based on PID
    chef.request_queue_id = request_queue_id;
    chef.response_queue_id = response_queue_id;
    chef.is_waiting_for_ingredients = 0;
    
    printf("[Chef %d] Chef process started\n", chef.id);
    
    // Main chef loop
    while (1) {
        // Check inventory and request ingredients if needed
        check_and_request_ingredients(&chef);
        
        // Check for confirmations from supply chain
        check_for_confirmations(&chef);
        
        // Try to prepare recipes
        prepare_recipes(&chef);
        
        // Small delay to prevent busy-waiting
        usleep(500000);  // 0.5 seconds
    }
    
    return 0;
}