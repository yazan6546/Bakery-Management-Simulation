#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "random.h"
#include "shared_mem_utils.h"
#include "customer.h"
#include "queue.h"

#define TEST_CUSTOMER_ID 123

int main(int argc, char *argv[]) {
    // Initialize random seed
    init_random();
    
    // Setup shared memory for the game
    Game *shared_game;
    setup_shared_memory(&shared_game);

    print_config(&shared_game->config);
    
    // Setup queue shared memory
    queue_shm *customer_queue;
    setup_queue_shared_memory(&customer_queue, shared_game->config.MAX_CUSTOMERS);

    if (customer_queue == NULL) {
        perror("Failed to setup customer queue shared memory");
        exit(EXIT_FAILURE);
    }
    
    // Initialize the queue
    customer_queue = initQueueShm(customer_queue, sizeof(Customer), 10);
    
    // Create a new customer with random attributes
    Customer new_customer;
    create_random_customer(&new_customer, &shared_game->config);

    printf("Created random customer with patience: %.4f, patience_decay: %.4f\n", 
           new_customer.patience, new_customer.patience_decay);
    
    // Add customer to queue
    if (queueShmEnqueue(customer_queue, &new_customer) == -1) {

        printf("capacity : %lu", customer_queue->capacity);
        printf("Failed to add customer to queue\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Added customer to queue successfully\n");
    
    // Get the position in the queue for this customer
    size_t pos = (customer_queue->head + customer_queue->count - 1) % customer_queue->capacity;
    Customer *queue_customer = &((Customer*)customer_queue->elements)[pos];
    
    int msg_queue_id = atoi(argv[1]);
    
    printf("Created message queue ID: %d\n", msg_queue_id);
    
    // Fork and exec the customer process
    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Child process (customer)
        char msg_id_str[16];
        char cust_id_str[16];
        char customer_str[32];
        
        snprintf(msg_id_str, sizeof(msg_id_str), "%d", msg_queue_id);
        snprintf(cust_id_str, sizeof(cust_id_str), "%d", TEST_CUSTOMER_ID);
        serialize_customer(&new_customer, customer_str);
        
        printf("Executing customer process with args: %s %s %s\n", 
               msg_id_str, cust_id_str, customer_str);
        
        execl(CUSTOMERS_BINARY_PATH, "./customers", msg_id_str, cust_id_str, customer_str, NULL);
        
        // If we reach here, execl failed
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        printf("Spawned customer with PID %d\n", pid);
        
        // Update customer PID in queue
        queue_customer->pid = pid;
        wait(NULL);

        
        // Clean up shared memory
        munmap(shared_game, sizeof(Game));
        munmap(customer_queue, queueShmSize(sizeof(Customer), shared_game->config.MAX_CUSTOMERS));
        
        // Exit
        return 0;
    }
}