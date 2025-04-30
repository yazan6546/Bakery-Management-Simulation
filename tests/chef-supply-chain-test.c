//
// Chef-Supply Chain Interaction Test
// Tests chef requesting ingredients from supply chains via message queues
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "../include/inventory.h"
#include "../include/chef.h"
#include "../include/supply_chain.h"
#include "../include/game.h"

#define SHM_NAME "/bakery_game_test"
#define TEST_NUM_CHEFS 2
#define TEST_NUM_SUPPLY_CHAINS 1
#define TEST_DURATION 15 // seconds - reduce for quicker test

// Global shared memory variables
Game *shared_game = NULL;
int shm_fd = -1;
sem_t* inventory_sem = NULL;
sem_t* ready_products_sem = NULL;

// Global message queue IDs
int request_queue_id = -1;
int response_queue_id = -1;

// Child process PIDs
pid_t chef_pids[TEST_NUM_CHEFS] = {0};
pid_t supply_chain_pids[TEST_NUM_SUPPLY_CHAINS] = {0};

// Signal handler for cleanup
void cleanup_handler(int signum) {
    printf("\n[Main] Cleaning up resources...\n");
    
    // Kill all child processes
    for (int i = 0; i < TEST_NUM_CHEFS; i++) {
        if (chef_pids[i] > 0) {
            kill(chef_pids[i], SIGTERM);
        }
    }
    
    for (int i = 0; i < TEST_NUM_SUPPLY_CHAINS; i++) {
        if (supply_chain_pids[i] > 0) {
            kill(supply_chain_pids[i], SIGTERM);
        }
    }
    
    // Clean up message queues
    if (request_queue_id != -1) {
        msgctl(request_queue_id, IPC_RMID, NULL);
    }
    
    if (response_queue_id != -1) {
        msgctl(response_queue_id, IPC_RMID, NULL);
    }
    
    // Clean up semaphores
    if (inventory_sem && ready_products_sem) {
        cleanup_semaphore_resources(inventory_sem, ready_products_sem);
    }
    
    // Clean up shared memory
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        munmap(shared_game, sizeof(Game));
    }
    
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }
    
    printf("[Main] Cleanup complete\n");
    exit(signum == SIGALRM ? 0 : signum);
}

// Initialize shared memory for the game
void init_shared_memory() {
    // Create shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(1);
    }
    
    // Set size of shared memory
    if (ftruncate(shm_fd, sizeof(Game)) == -1) {
        perror("Failed to set size of shared memory");
        cleanup_handler(1);
    }
    
    // Map shared memory
    shared_game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_game == MAP_FAILED) {
        perror("Failed to map shared memory");
        cleanup_handler(1);
    }
    
    // Initialize game structures
    init_inventory(&shared_game->inventory);
    init_ready_products(&shared_game->ready_products);
    
    printf("[Main] Shared memory initialized\n");
}

// Initialize message queues
void init_message_queues() {
    // Create request queue
    request_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (request_queue_id == -1) {
        perror("Failed to create request queue");
        cleanup_handler(1);
    }
    
    // Create response queue
    response_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (response_queue_id == -1) {
        perror("Failed to create response queue");
        cleanup_handler(1);
    }
    
    printf("[Main] Message queues initialized (request_queue=%d, response_queue=%d)\n", 
           request_queue_id, response_queue_id);
}

// Start a chef process
pid_t start_chef(int chef_id) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for chef");
        return -1;
    }
    
    if (pid == 0) {
        // Child process - chef
        srand(time(NULL) ^ getpid());
        
        // Set up semaphores
        sem_t* chef_inventory_sem = setup_inventory_semaphore();
        if (chef_inventory_sem == NULL) {
            perror("Failed to setup inventory semaphore in chef");
            exit(1);
        }
        
        sem_t* chef_ready_products_sem = setup_ready_products_semaphore();
        if (chef_ready_products_sem == NULL) {
            perror("Failed to setup ready products semaphore in chef");
            exit(1);
        }
        
        // Initialize chef state
        ChefState chef;
        chef.id = chef_id;
        chef.request_queue_id = request_queue_id;
        chef.response_queue_id = response_queue_id;
        chef.is_waiting_for_ingredients = 0;
        chef.inventory_sem = chef_inventory_sem;
        chef.ready_products_sem = chef_ready_products_sem;
        
        printf("[Chef %d] Chef process started\n", chef.id);
        
        // Main chef loop - simplified for the test
        int count = 0;
        while (1) {
            // Check inventory and request ingredients if needed
            check_and_request_ingredients(&chef, &shared_game->inventory);
            
            // Check for confirmations from supply chain
            check_for_confirmations(&chef);
            
            // Try to prepare recipes
            prepare_recipes(&chef, &shared_game->inventory, &shared_game->ready_products);
            
            // Small delay
            usleep(500000);  // 0.5 seconds
            
            // Count iterations to occasionally print status
            if (++count % 10 == 0) {
                printf("[Chef %d] Still running, waiting=%d\n", 
                       chef.id, chef.is_waiting_for_ingredients);
            }
        }
        
        // This will never execute due to the infinite loop
        exit(0);
    }
    
    return pid;
}

// Start a supply chain process
pid_t start_supply_chain(int sc_id) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for supply chain");
        return -1;
    }
    
    if (pid == 0) {
        // Child process - supply chain
        srand(time(NULL) ^ (getpid() + 100));
        
        // Setup inventory semaphore
        sem_t* sc_inventory_sem = setup_inventory_semaphore();
        if (sc_inventory_sem == NULL) {
            perror("Failed to setup inventory semaphore in supply chain");
            exit(1);
        }
        
        // Initialize supply chain state
        SupplyChainState state;
        state.id = sc_id;
        state.request_queue_id = request_queue_id;
        state.response_queue_id = response_queue_id;
        state.inventory_sem = sc_inventory_sem;
        
        printf("[Supply Chain %d] Supply chain process started\n", sc_id);
        
        // Main loop - simplified for the test
        while (1) {
            // Wait for a restock request from chef
            RestockRequest chef_request;
            
            // Check queue status occasionally
            struct msqid_ds queue_stat;
            msgctl(request_queue_id, IPC_STAT, &queue_stat);
            
            if (queue_stat.msg_qnum == 0) {
                // Print status message occasionally when no requests are pending
                static time_t last_status = 0;
                time_t now = time(NULL);
                if (now - last_status >= 5) {
                    printf("[Supply Chain %d] Waiting for requests...\n", sc_id);
                    last_status = now;
                }
            }
            
            // Try to receive a message (non-blocking for test purposes)
            if (msgrcv(request_queue_id, &chef_request, sizeof(RestockRequest) - sizeof(long), 0, IPC_NOWAIT) == -1) {
                if (errno != ENOMSG) {
                    perror("[Supply Chain] Error receiving message");
                }
                sleep(1);  // Sleep 1 second before trying again
                continue;
            }
            
            // Convert chef request to supply chain format
            SupplyChainRequest supply_request;
            supply_request.mtype = chef_request.mtype;
            supply_request.ingredient = chef_request.ingredient;
            supply_request.quantity = chef_request.quantity;
            supply_request.urgency = chef_request.urgency;
            
            // Process the request
            process_restock_request(&state, &supply_request, &shared_game->inventory);
        }
        
        // This will never execute due to the infinite loop
        exit(0);
    }
    
    return pid;
}

// Print the current state of the inventory and ready products
void print_inventory_status() {
    // Create and get local semaphores for reading
    sem_t* inv_sem = setup_inventory_semaphore();
    sem_t* ready_sem = setup_ready_products_semaphore();
    
    if (!inv_sem || !ready_sem) {
        printf("[Main] Error setting up semaphores for status check\n");
        return;
    }
    
    // Lock inventory for read
    lock_inventory(inv_sem);
    
    // Print inventory status
    printf("\n-------- Current Inventory Status --------\n");
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        printf("Ingredient %d: %d units\n", i, shared_game->inventory.quantities[i]);
    }
    
    unlock_inventory(inv_sem);
    
    // Lock ready products for read
    lock_ready_products(ready_sem);
    
    // Print ready products status
    printf("\n-------- Ready Products Status --------\n");
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        printf("Product %d: %d units\n", i, shared_game->ready_products.quantities[i]);
    }
    
    printf("\n");
    
    unlock_ready_products(ready_sem);
    
    // Close semaphores
    sem_close(inv_sem);
    sem_close(ready_sem);
}

// Main function
int main() {
    printf("=== Chef-Supply Chain Interaction Test ===\n\n");
    
    // Register signal handler for cleanup
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    
    // Initialize shared memory
    init_shared_memory();
    
    // Setup semaphores
    inventory_sem = setup_inventory_semaphore();
    ready_products_sem = setup_ready_products_semaphore();
    
    if (!inventory_sem || !ready_products_sem) {
        printf("Failed to setup semaphores\n");
        cleanup_handler(1);
    }
    
    // Initialize message queues
    init_message_queues();
    
    // Initialize the inventory with some initial ingredients
    int initial_quantities[NUM_INGREDIENTS] = {0};
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        initial_quantities[i] = 5; // Start with low quantities to trigger requests
    }
    add_ingredients(&shared_game->inventory, initial_quantities, inventory_sem);
    
    // Start supply chain processes
    printf("Starting %d supply chain processes...\n", TEST_NUM_SUPPLY_CHAINS);
    for (int i = 0; i < TEST_NUM_SUPPLY_CHAINS; i++) {
        supply_chain_pids[i] = start_supply_chain(i);
        if (supply_chain_pids[i] == -1) {
            perror("Failed to start supply chain process");
            cleanup_handler(1);
        }
        printf("Started supply chain process %d with PID %d\n", i, supply_chain_pids[i]);
    }
    
    // Give supply chains a moment to initialize
    sleep(1);
    
    // Start chef processes
    printf("Starting %d chef processes...\n", TEST_NUM_CHEFS);
    for (int i = 0; i < TEST_NUM_CHEFS; i++) {
        chef_pids[i] = start_chef(i);
        if (chef_pids[i] == -1) {
            perror("Failed to start chef process");
            cleanup_handler(1);
        }
        printf("Started chef process %d with PID %d\n", i, chef_pids[i]);
    }
    
    // Set up alarm for test duration
    printf("\nTest will run for %d seconds...\n\n", TEST_DURATION);
    signal(SIGALRM, cleanup_handler);
    alarm(TEST_DURATION);
    
    // Monitor and print status periodically
    for (int i = 0; i < TEST_DURATION; i += 5) {
        sleep(5); // Check every 5 seconds
        print_inventory_status();
    }
    
    // Clean up (this should not execute due to alarm signal)
    cleanup_handler(0);
    
    return 0;
}