#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "customer.h"
#include "game.h"
#include "queue.h"
#include "random.h"

// Global variables
Game *shared_game;
queue_shm *customer_queue;
int msg_queue_id;
int active_customers = 0;

// Simplified message structure - only for leave notifications
typedef struct {
    long mtype;
    pid_t customer_pid;
} LeaveMessage;

void cleanup_resources() {
    // Clean up message queue
    if (msg_queue_id > 0) {
        msgctl(msg_queue_id, IPC_RMID, NULL);
    }
    // Kill any remaining customer processes
    for (int i = 0; i < customer_queue->count; i++) {
        Customer *c = (Customer*)(customer_queue->elements + (i * sizeof(Customer)));
        if (c->pid > 0) {
            kill(c->pid, SIGTERM);
        }
    }

    // Detach shared memory
    if (shared_game) {
        munmap(shared_game, sizeof(Game));
    }
}

// Handle customer leave notifications
void handle_customer_message(int signum) {
    LeaveMessage msg;

    // Try to receive messages from customers (non-blocking)
    while (msgrcv(msg_queue_id, &msg, sizeof(pid_t), 1, IPC_NOWAIT) != -1) {
        pid_t pid = msg.customer_pid;


            // Find and remove customer from queue
        for (int i = 0; i < customer_queue->count; i++) {
            Customer *c = (Customer*)(customer_queue->elements + (i * sizeof(Customer)));
            if (c->pid == pid) {
                printf("Removing customer (PID %d) from queue\n", pid);
                queueShmRemoveAt(customer_queue, i);
                break;
            }
        }
        active_customers--;
    }
}


void spawn_customer(int customer_id) {
    if (active_customers >= shared_game->config.MAX_CUSTOMERS) {
        return; // Don't spawn if we're at max capacity
    }

    Customer new_customer;
    // Create a new customer with random attributes
    create_random_customer(&new_customer, &shared_game->config);

    // Add to queue first (safer to do this before fork)
    if (!queueShmEnqueue(customer_queue, &new_customer)) {
        printf("Failed to add customer to queue\n");
        return;
    }

    // Get pointer to the customer in the queue
    Customer* queue_customer = (Customer*)(customer_queue->elements +
                                             ((customer_queue->count - 1) * sizeof(Customer)));

    // Fork a new process for the customer
    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork customer process");
        queueShmDequeue(customer_queue, NULL);  // Remove from queue if fork failed
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process (customer)
        char msg_id_str[16];
        char cust_id_str[16];
        char queue_offset_str[16];

        // Calculate offset of this customer in the queue
        size_t offset = (queue_customer - (Customer*)customer_queue->elements);

        snprintf(msg_id_str, sizeof(msg_id_str), "%d", msg_queue_id);
        snprintf(cust_id_str, sizeof(cust_id_str), "%d", customer_id);
        snprintf(queue_offset_str, sizeof(queue_offset_str), "%zu", offset);

        execl("./customer", "./customer", msg_id_str, cust_id_str, queue_offset_str, NULL);

        // If we reach here, execl failed
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process (customer manager)
        printf("Spawned customer %d with PID %d\n", customer_id, pid);

        // Update customer PID in queue
        queue_customer->pid = pid;
        active_customers++;
    }
}

int main(int argc, char *argv[]) {
    // Setup shared memory for game and customer queue
    setup_shared_memory(&customer_queue, &shared_game);

    // Initialize random number generator
    init_random();

    // Setup signal handler for customer messages
    signal(SIGUSR1, handle_customer_message);

    // Register cleanup handler
    atexit(cleanup_resources);

    // Create message queue for communication (only for leave notifications)
    msg_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    printf("Customer Manager started. Message queue ID: %d\n", msg_queue_id);

    int next_customer_id = 0;

    // Main loop
    while (check_game_conditions(shared_game)) {

        // Spawn new customers if we're below max
        if (active_customers < shared_game->config.MAX_CUSTOMERS) {
            // Random chance to spawn a new customer
            if (random_float(0, 1) < 0.3) { // 30% chance each iteration
                spawn_customer(next_customer_id++);
            }
        }

        // Sleep for a random time between 1-3 seconds
        sleep(1 + rand() % 3);
    }

    return 0;
}