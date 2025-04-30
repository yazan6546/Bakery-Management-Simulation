//
// Inventory Semaphore Test
// Tests concurrent access to shared inventory using semaphores
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include "../include/inventory.h"

#define SHM_NAME "/bakery_inventory"
#define SHM_READY_NAME "/bakery_ready_products"
#define NUM_PROCESSES 5
#define NUM_OPERATIONS 10

void producer_process(Inventory *shared_inventory, sem_t *sem, int producer_id) {
    printf("Producer %d started\n", producer_id);
    srand(time(NULL) ^ (producer_id * 1000));
    
    // Perform a series of operations
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        // Create random batch of ingredients
        int quantities[NUM_INGREDIENTS] = {0};
        for (int j = 0; j < NUM_INGREDIENTS; j++) {
            quantities[j] = rand() % 5;  // 0-4 units of each ingredient
        }
        
        // Add ingredients to inventory
        add_ingredients(shared_inventory, quantities, sem);
        
        printf("Producer %d: Added ingredients batch %d\n", producer_id, i+1);
        
        // Sleep for a random time to simulate processing
        usleep((rand() % 500 + 100) * 1000);  // 100-600ms
    }
    
    printf("Producer %d finished\n", producer_id);
}

void consumer_process(Inventory *shared_inventory, ReadyProducts *ready_products, 
                      sem_t *inv_sem, sem_t *ready_sem, int consumer_id) {
    printf("Consumer %d started\n", consumer_id);
    srand(time(NULL) ^ (consumer_id * 2000));
    
    // Perform a series of operations
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        // Define a recipe (ingredients needed)
        int recipe[NUM_INGREDIENTS] = {0};
        for (int j = 0; j < 3; j++) {  // Use 3 random ingredients
            int ingredient = rand() % NUM_INGREDIENTS;
            recipe[ingredient] = rand() % 3 + 1;  // 1-3 units of the ingredient
        }
        
        // Try to use ingredients from inventory
        if (check_ingredients(shared_inventory, recipe, inv_sem)) {
            // Use ingredients
            use_ingredients(shared_inventory, recipe, inv_sem);
            printf("Consumer %d: Used ingredients for recipe %d\n", consumer_id, i+1);
            
            // Create a product
            ProductType product_type = rand() % NUM_PRODUCTS;
            
            // Add to ready products
            add_ready_product(ready_products, product_type, 1, ready_sem);
            printf("Consumer %d: Added product type %d\n", consumer_id, product_type);
        } else {
            printf("Consumer %d: Not enough ingredients for recipe %d\n", consumer_id, i+1);
        }
        
        // Sleep for a random time to simulate processing
        usleep((rand() % 300 + 100) * 1000);  // 100-400ms
    }
    
    printf("Consumer %d finished\n", consumer_id);
}

int main() {
    // Set up shared memory for inventory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed for inventory");
        exit(1);
    }
    
    if (ftruncate(shm_fd, sizeof(Inventory)) == -1) {
        perror("ftruncate failed for inventory");
        exit(1);
    }
    
    Inventory *shared_inventory = mmap(NULL, sizeof(Inventory), 
                                      PROT_READ | PROT_WRITE, MAP_SHARED, 
                                      shm_fd, 0);
    if (shared_inventory == MAP_FAILED) {
        perror("mmap failed for inventory");
        exit(1);
    }
    
    // Set up shared memory for ready products
    int shm_ready_fd = shm_open(SHM_READY_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_ready_fd == -1) {
        perror("shm_open failed for ready products");
        exit(1);
    }
    
    if (ftruncate(shm_ready_fd, sizeof(ReadyProducts)) == -1) {
        perror("ftruncate failed for ready products");
        exit(1);
    }
    
    ReadyProducts *shared_ready_products = mmap(NULL, sizeof(ReadyProducts), 
                                               PROT_READ | PROT_WRITE, MAP_SHARED, 
                                               shm_ready_fd, 0);
    if (shared_ready_products == MAP_FAILED) {
        perror("mmap failed for ready products");
        exit(1);
    }
    
    // Initialize the shared structures
    init_inventory(shared_inventory);
    init_ready_products(shared_ready_products);
    
    // Setup semaphores
    sem_t* inventory_sem = setup_inventory_semaphore();
    sem_t* ready_products_sem = setup_ready_products_semaphore();
    
    if (!inventory_sem || !ready_products_sem) {
        fprintf(stderr, "Failed to set up semaphores\n");
        exit(1);
    }
    
    printf("Starting test with %d processes, %d operations each\n", 
           NUM_PROCESSES * 2, NUM_OPERATIONS);
    
    pid_t pids[NUM_PROCESSES * 2];
    int child_count = 0;
    
    // Create producer processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[child_count] = fork();
        if (pids[child_count] < 0) {
            perror("fork failed");
            exit(1);
        } else if (pids[child_count] == 0) {
            // Child process (producer)
            producer_process(shared_inventory, inventory_sem, i + 1);
            exit(0);
        }
        child_count++;
    }
    
    // Create consumer processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[child_count] = fork();
        if (pids[child_count] < 0) {
            perror("fork failed");
            exit(1);
        } else if (pids[child_count] == 0) {
            // Child process (consumer)
            consumer_process(shared_inventory, shared_ready_products, 
                             inventory_sem, ready_products_sem, i + 1);
            exit(0);
        }
        child_count++;
    }
    
    // Wait for all child processes to complete
    for (int i = 0; i < child_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    
    // Print final state
    printf("\nFinal Inventory State:\n");
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        printf("Ingredient %d: %d\n", i, shared_inventory->quantities[i]);
    }
    
    printf("\nFinal Ready Products State:\n");
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        printf("Product %d: %d\n", i, shared_ready_products->quantities[i]);
    }
    
    // Clean up
    cleanup_semaphore_resources(inventory_sem, ready_products_sem);
    
    if (munmap(shared_inventory, sizeof(Inventory)) == -1) {
        perror("munmap failed for inventory");
    }
    
    if (munmap(shared_ready_products, sizeof(ReadyProducts)) == -1) {
        perror("munmap failed for ready products");
    }
    
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed for inventory");
    }
    
    if (shm_unlink(SHM_READY_NAME) == -1) {
        perror("shm_unlink failed for ready products");
    }
    
    printf("Test completed successfully\n");
    return 0;
}