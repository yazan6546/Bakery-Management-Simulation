//
// Created by yazan on 4/26/2025.
//

#ifndef SUPPLY_CHAIN_H
#define SUPPLY_CHAIN_H

#include <semaphore.h>

// Message structures for supply chain communication
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int urgency; // 0-10 scale, 10 being most urgent
} SupplyChainRequest;

typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int success; // 1 if successful, 0 if failed
} SupplyChainConfirmation;

// Supply chain state structure
typedef struct {
    int id;
    int request_queue_id;
    int response_queue_id;
    sem_t* inventory_sem;
} SupplyChainState;

// Function declarations
void process_restock_request(SupplyChainState *state, SupplyChainRequest *request, Inventory *inventory);

#endif // SUPPLY_CHAIN_H