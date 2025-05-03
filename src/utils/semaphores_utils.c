#include "semaphores_utils.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

// Setup semaphore using POSIX semaphores
sem_t* setup_inventory_semaphore() {
    // First try to unlink any existing semaphore (ignore errors if it doesn't exist)
    sem_unlink(SEM_NAME);
    
    // Create or open the semaphore
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); // Initial value 1
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return NULL;
    }
    printf("Inventory semaphore created successfully\n");
    return sem;
}

// Setup semaphore for ready products
sem_t* setup_ready_products_semaphore(void) {
    // First try to unlink any existing semaphore (ignore errors if it doesn't exist)
    sem_unlink(READY_SEM_NAME);
    
    // Create or open the semaphore
    sem_t* sem = sem_open(READY_SEM_NAME, O_CREAT, 0666, 1); // Initial value 1
    if (sem == SEM_FAILED) {
        perror("sem_open failed for ready products");
        return NULL;
    }
    printf("Ready products semaphore created successfully\n");
    return sem;
}

// Lock the inventory for exclusive access
void lock_inventory(sem_t* sem) {
    if (sem_wait(sem) == -1) {
        perror("sem_wait failed");
        exit(1);
    }
}

// Unlock the inventory
void unlock_inventory(sem_t* sem) {
    if (sem_post(sem) == -1) {
        perror("sem_post failed");
        exit(1);
    }
}

// Unlock the ready products
void unlock_ready_products(sem_t* sem) {
    if (sem_post(sem) == -1) {
        perror("sem_post failed for ready products");
        exit(1);
    }
}

// Lock the ready products for exclusive access
void lock_ready_products(sem_t* sem) {
    if (sem_wait(sem) == -1) {
        perror("sem_wait failed for ready products");
        exit(1);
    }
}

// Cleanup resources
void cleanup_ready_products_semaphore_resources(sem_t* ready_products_sem) {
    sem_close(ready_products_sem);
    sem_unlink(READY_SEM_NAME);
}

void cleanup_inventory_semaphore_resources(sem_t* inventory_sem) {
    sem_close(inventory_sem);
    sem_unlink(SEM_NAME);
}

// Reset all known semaphores in the application
void reset_all_semaphores() {
    // Unlink all named semaphores used in the application
    printf("Resetting all semaphores...\n");
    
    // Main semaphores
    sem_unlink(SEM_NAME);
    sem_unlink(READY_SEM_NAME);
    
    // Any other semaphores used in the application
    sem_unlink(COMPLAINT_SEM_NAME);
    
    printf("All semaphores reset\n");
}
