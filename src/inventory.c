//
// Created by yazan on 4/26/2025.
//
#include "inventory.h"
#include <string.h>
#include <unistd.h>

// Global variables for shared memory and semaphores
int shm_fd = -1;
Inventory* shared_inventory = NULL;
sem_t* inventory_sem = NULL;

// Setup shared memory for inventory using mmap
int setup_shared_memory(void) {
    // Create or open a shared memory file
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return -1;
    }

    // Set the size of the shared memory region
    if (ftruncate(shm_fd, sizeof(Inventory)) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    // Map the shared memory region into our address space
    shared_inventory = (Inventory*)mmap(NULL, sizeof(Inventory), 
                                        PROT_READ | PROT_WRITE, MAP_SHARED, 
                                        shm_fd, 0);
    if (shared_inventory == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    // Initialize the inventory in shared memory
    init_inventory(shared_inventory);
    return 0;
}

// Setup semaphore using POSIX semaphores
int setup_semaphore(void) {
    // Create or open the semaphore
    inventory_sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); // Initial value 1
    if (inventory_sem == SEM_FAILED) {
        perror("sem_open failed");
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

// Clean up resources
void cleanup_resources(void) {
    // Unmap shared memory
    if (shared_inventory != NULL && shared_inventory != MAP_FAILED) {
        if (munmap(shared_inventory, sizeof(Inventory)) == -1) {
            perror("munmap failed");
        }
    }

    // Close and unlink shared memory
    if (shm_fd != -1) {
        close(shm_fd);
        if (shm_unlink(SHM_NAME) == -1) {
            perror("shm_unlink failed");
        }
    }

    // Close and unlink semaphore
    if (inventory_sem != NULL && inventory_sem != SEM_FAILED) {
        sem_close(inventory_sem);
        if (sem_unlink(SEM_NAME) == -1) {
            perror("sem_unlink failed");
        }
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

// Add ingredient with thread safety
void add_ingredient(Inventory *inventory, IngredientType type, int quantity) {
    if (inventory == shared_inventory) {
        lock_inventory();
    }
    
    if (type >= 0 && type < NUM_INGREDIENTS) {
        inventory->quantities[type] += quantity;
    }
    
    if (inventory == shared_inventory) {
        unlock_inventory();
    }
}

void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    if (inventory == shared_inventory) {
        lock_inventory();
    }
    
    // Add quantities from the array
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] += quantities[i];
    }
    
    if (inventory == shared_inventory) {
        unlock_inventory();
    }
}

int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    int result = 1;
    
    if (inventory == shared_inventory) {
        lock_inventory();
    }
    
    // Check if we have enough of each ingredient
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        if (inventory->quantities[i] < quantities[i]) {
            result = 0; // Not enough ingredients
            break;
        }
    }
    
    if (inventory == shared_inventory) {
        unlock_inventory();
    }
    
    return result;
}

void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]) {
    if (inventory == shared_inventory) {
        lock_inventory();
    }
    
    // Deduct used ingredients
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] -= quantities[i];
    }
    
    if (inventory == shared_inventory) {
        unlock_inventory();
    }
}

void restock_ingredients(Inventory *inventory) {
    if (inventory == shared_inventory) {
        lock_inventory();
    }
    
    // Reset all ingredients to 0
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
    
    if (inventory == shared_inventory) {
        unlock_inventory();
    }
}

