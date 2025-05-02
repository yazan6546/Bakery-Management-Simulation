//
// Created by yazan on 4/26/2025.
//

#ifndef SUPPLY_CHAIN_H
#define SUPPLY_CHAIN_H

#include <semaphore.h>

// Message structures for supply chain communication
typedef struct {
    long mtype; // Message type
    Ingredient ingredients[NUM_INGREDIENTS]; // Ingredient data
} SupplyChainMessage;

#endif // SUPPLY_CHAIN_H