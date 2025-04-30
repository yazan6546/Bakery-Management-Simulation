//
// Created by yazan on 4/26/2025.
// Supply chain functions extracted for testing
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
#include "supply_chain.h"
#include "chef.h"


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