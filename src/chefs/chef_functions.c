//
// Created by yazan on 4/26/2025.
// Chef functions extracted for testing
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

// Function to check if ingredients are low and need to be restocked
void check_and_request_ingredients(ChefState *chef, Inventory *inventory) {
    // Lock inventory to check
    lock_inventory(chef->inventory_sem);
    
    // Check each ingredient
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        // If ingredient is below threshold, request restock
        if (inventory->quantities[i] < LOW_INGREDIENT_THRESHOLD && !chef->is_waiting_for_ingredients) {
            int quantity_needed = RESTOCK_TARGET_QUANTITY - inventory->quantities[i]; // Restock to target
            if (quantity_needed <= 0) continue;
            
            printf("[Chef %d] Ingredient %d is low (%d). Requesting restock.\n",
                   chef->id, i, inventory->quantities[i]);
            
            // Create restock request
            RestockRequest request;
            request.mtype = 1; // Default message type
            request.ingredient = i;
            request.quantity = quantity_needed;
            
            // Calculate urgency based on how low we are
            request.urgency = 10 - (inventory->quantities[i] / 5);
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
    
    unlock_inventory(chef->inventory_sem);
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
void prepare_recipes(ChefState *chef, Inventory *inventory, ReadyProducts *ready_products) {
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
    if (check_ingredients(inventory, ingredients_needed, chef->inventory_sem)) {
        // Use ingredients
        use_ingredients(inventory, ingredients_needed, chef->inventory_sem);
        
        // Add to ready products based on recipe type
        switch (recipe_choice) {
            case 0:
                add_ready_product(ready_products, BREAD, 1, chef->ready_products_sem);
                printf("[Chef %d] Prepared bread\n", chef->id);
                break;
                
            case 1:
                add_ready_product(ready_products, CAKE, 1, chef->ready_products_sem);
                printf("[Chef %d] Prepared cake\n", chef->id);
                break;
                
            case 2:
                add_ready_product(ready_products, SANDWICH, 1, chef->ready_products_sem);
                printf("[Chef %d] Prepared sandwich\n", chef->id);
                break;
        }
        
        // Cook time
        sleep(2);
    }
}