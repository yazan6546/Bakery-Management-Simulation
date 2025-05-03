#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "shared_mem_utils.h"
#include "semaphores_utils.h"
#include "game.h"
#include "queue.h"
#include "seller.h"
#include "bakery_message.h"
#include "customer.h"

// Global variables
Seller seller;
Game *shared_game;
queue_shm *customer_queue;
sem_t *queue_sem;
int running = 1;
int msg_queue_id;

void handle_sigint(int sig) {
    printf("Seller %d received SIGINT, exiting...\n", seller.id);
    running = 0;
}

void process_customer_order(pid_t customer_pid, CustomerOrder *order, Game *shared_game) {
    printf("Seller %d: Processing order from customer PID %d with %d items, total price: %.2f\n",
           seller.id, customer_pid, order->item_count, order->total_price);

    if (!check_and_fulfill_order(&shared_game->ready_products, order, NULL)) {
        printf("Seller %d: Order could not be fulfilled\n", seller.id);
        // Handle order failure (e.g., notify customer)
        CompletionMessage compl_msg;
        compl_msg.mtype = customer_pid;  // Use customer's PID as message type
        compl_msg.result = ORDER_FAILED;
        compl_msg.total_price = 0.0f;
        if (msgsnd(msg_queue_id, &compl_msg, sizeof(CompletionMessage) - sizeof(long), 0) == -1) {
            perror("Failed to send completion message");
        }

        return;
    }

    // Send completion message using customer's PID as message type
    CompletionMessage compl_msg;
    compl_msg.mtype = customer_pid;  // Use customer's PID as message type
    compl_msg.result = ORDER_SUCCESS;
    compl_msg.total_price = order->total_price;

    if (msgsnd(msg_queue_id, &compl_msg, sizeof(CompletionMessage) - sizeof(long), 0) == -1) {
        perror("Failed to send completion message");
    }

    // Update game statistics
    shared_game->daily_profit += order->total_price;
}

void serve_customer(Customer *customer) {
    printf("Seller %d: Serving customer %d (PID %d)\n", seller.id, customer->id, customer->pid);

    // Update seller state
    seller.state = TAKING_ORDER;

    // Send signal to customer that it's their turn
    kill(customer->pid, SIGUSR1);
    printf("Seller %d: Signaled customer %d\n", seller.id, customer->id);

    // Wait for customer to send order through message queue
    OrderMessage order_msg;
    if (msgrcv(msg_queue_id, &order_msg, sizeof(OrderMessage) - sizeof(long), seller.pid, 0) != -1) {
        // Update seller state
        seller.state = PROCESSING_ORDER;

        // Process the order
        process_customer_order(customer->pid, &order_msg.order, shared_game);

        // Update seller state
        seller.state = COMPLETING_ORDER;
    } else {
        perror("Failed to receive order from customer");
    }

    // Back to idle state
    seller.state = IDLE;
}

void seller_loop() {
    while (running) {
        // Try to get a customer from the queue
        Customer customer;

        // Lock the queue with semaphore
        sem_wait(queue_sem);

        if (!queueShmIsEmpty(customer_queue)) {
            // Dequeue a customer
            if (queueShmDequeue(customer_queue, &customer) == 0) {
                printf("Seller %d: Dequeued customer with PID %d\n", seller.id, customer.pid);

                // Release the queue semaphore
                sem_post(queue_sem);

                // Serve the customer
                serve_customer(&customer);
            } else {
                sem_post(queue_sem);
                printf("Seller %d: Failed to dequeue customer\n", seller.id);
            }
        } else {
            sem_post(queue_sem);
            printf("Seller %d: No customers in queue, waiting...\n", seller.id);
            sleep(1);
        }

        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    // if (argc < 2) {
    //     fprintf(stderr, "Usage: %s <seller_id>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // int seller_id = atoi(argv[1]);
    int seller_id = 1; // For testing purposes, hardcoded seller ID

    init_seller(&seller, seller_id);
    printf("Seller %d initialized with PID %d\n", seller.id, seller.pid);

    // Set up signal handler
    signal(SIGINT, handle_sigint);

    // Set up shared memory for game state
    setup_shared_memory(&shared_game);

    // Set up shared memory for customer queue
    setup_queue_shared_memory(&customer_queue, shared_game->config.MAX_CUSTOMERS);

    // Open the queue semaphore
    queue_sem = sem_open(QUEUE_SEM_NAME, O_CREAT, 0666, 1);
    if (queue_sem == SEM_FAILED) {
        perror("Failed to open queue semaphore");
        exit(EXIT_FAILURE);
    }

    // Get the global message queue ID - use the same one as customer_manager
    int msg_queue_id = get_message_queue();

    // Start seller loop
    seller_loop();

    // Cleanup
    sem_close(queue_sem);
    munmap(shared_game, sizeof(Game));

    return 0;
}