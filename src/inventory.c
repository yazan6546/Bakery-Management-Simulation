//
// Created by yazan on 4/26/2025.
//
#include "inventory.h"

void init_inventory(Inventory *inventory) {
    // Initialize all quantities to zero
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
    inventory->max_capacity = 100; // Set a default max capacity
}

void add_ingredient(Inventory *inventory, IngredientType type, int quantity) {
    if (type >= 0 && type < NUM_INGREDIENTS) {
        inventory->quantities[type] += quantity;
    }
}

void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    // Add quantities from the array
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] += quantities[i];
    }
}

int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    // Check if we have enough of each ingredient
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        if (inventory->quantities[i] < quantities[i]) {
            return 0; // Not enough ingredients
        }
    }
    return 1; // Enough ingredients
}

void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    // Deduct used ingredients
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] -= quantities[i];
    }
}

void restock_ingredients(Inventory *inventory) {
    // Reset all ingredients to 0
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
}

