//
// Created by yazan on 4/26/2025.
//
#include "inventory.h"
#include <string.h>
#include <unistd.h>

// Global variables for shared memory and semaphores
sem_t* inventory_sem = NULL;
sem_t* ready_products_sem = NULL;


// Setup semaphore using POSIX semaphores
int setup_inventory_semaphore(void) {
    // Create or open the semaphore
    inventory_sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); // Initial value 1
    if (inventory_sem == SEM_FAILED) {
        perror("sem_open failed");
        return -1;
    }
    return 0;
}

// Setup semaphore for ready products
int setup_ready_products_semaphore(void) {
    // Create or open the semaphore
    ready_products_sem = sem_open(READY_SEM_NAME, O_CREAT, 0666, 1); // Initial value 1
    if (ready_products_sem == SEM_FAILED) {
        perror("sem_open failed for ready products");
        return -1;
    }
    return 0;
}

// Lock the inventory for exclusive access
void lock_inventory(void) {
    if (sem_wait(inventory_sem) == -1) {
        perror("sem_wait failed");
        exit(1);
    }
}

// Unlock the inventory
void unlock_inventory(void) {
    if (sem_post(inventory_sem) == -1) {
        perror("sem_post failed");
        exit(1);
    }
}

// Lock the ready products for exclusive access
void lock_ready_products(void) {
    if (sem_wait(ready_products_sem) == -1) {
        perror("sem_wait failed for ready products");
        exit(1);
    }
}

// Unlock the ready products
void unlock_ready_products(void) {
    if (sem_post(ready_products_sem) == -1) {
        perror("sem_post failed for ready products");
        exit(1);
    }
}

// Unlock the ready products
void unlock_ready_products(void) {
    if (sem_post(ready_products_sem) == -1) {
        perror("sem_post failed for ready products");
        exit(1);
    }
}

 

// Initialize inventory
void init_inventory(Inventory *inventory) {
    // Initialize all quantities to zero
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
    inventory->max_capacity = 100; // Set a default max capacity
}

// Initialize ready products
void init_ready_products(ReadyProducts *ready_products) {
    // Initialize all quantities to zero
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        ready_products->quantities[i] = 0;
    }
    ready_products->max_capacity = 50; // Set a default max capacity
}

// Add ingredient with thread safety
void add_ingredient(Inventory *inventory, IngredientType type, int quantity) {
    lock_inventory();
    
    if (type >= 0 && type < NUM_INGREDIENTS) {
        inventory->quantities[type] += quantity;
    }
    
    unlock_inventory();
}

void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    lock_inventory();
    
    // Add quantities from the array
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] += quantities[i];
    }
    
    unlock_inventory();
}

int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    int result = 1;
    
    lock_inventory();
    
    // Check if we have enough of each ingredient
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        if (inventory->quantities[i] < quantities[i]) {
            result = 0; // Not enough ingredients
            break;
        }
    }
    
    unlock_inventory();
    
    return result;
}

void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    lock_inventory();
    
    // Deduct used ingredients
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] -= quantities[i];
    }
    
    unlock_inventory();
}

void restock_ingredients(Inventory *inventory) {
    lock_inventory();
    
    // Reset all ingredients to 0
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
    
    unlock_inventory();
}

// Add ready product with thread safety
void add_ready_product(ReadyProducts *ready_products, ProductType type, int quantity) {
    lock_ready_products();
    
    if (type >= 0 && type < NUM_PRODUCTS) {
        ready_products->quantities[type] += quantity;
    }
    
    unlock_ready_products();
}

// Get ready product with thread safety
// Returns 1 if successful, 0 if not enough products
int get_ready_product(ReadyProducts *ready_products, ProductType type, int quantity) {
    int result = 0;
    
    lock_ready_products();
    
    if (type >= 0 && type < NUM_PRODUCTS && ready_products->quantities[type] >= quantity) {
        ready_products->quantities[type] -= quantity;
        result = 1;
    }
    
    unlock_ready_products();
    
    return result;
}

// Add ready product with thread safety
void add_ready_product(ReadyProducts *ready_products, ProductType type, int quantity) {
    
    lock_ready_products();
    
    
    if (type >= 0 && type < NUM_PRODUCTS) {
        ready_products->quantities[type] += quantity;
    }
    
    
    unlock_ready_products();
    
}

// Get ready product with thread safety
// Returns 1 if successful, 0 if not enough products
int get_ready_product(ReadyProducts *ready_products, ProductType type, int quantity) {
    int result = 0;
    
    
    lock_ready_products();
    
    
    if (type >= 0 && type < NUM_PRODUCTS && ready_products->quantities[type] >= quantity) {
        ready_products->quantities[type] -= quantity;
        result = 1;
    }
    
    
    unlock_ready_products();
    
    
    return result;
}

