#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "game.h"
#include "queue.h"
#include "random.h"
#include "bakery_message.h"
#include "time.h"
#include "semaphores_utils.h"
#include "shared_mem_utils.h"

// Global variables
Game *shared_game;
queue_shm *customer_queue;
int msg_queue_id;
int max_customers = 0;
int active_customers = 0;
sem_t *complaint_sem = NULL;

void cleanup_resources() {
    printf("Cleaning up resources...in customer_manager\n");
    if (msg_queue_id > 0) {
        msgctl(msg_queue_id, IPC_RMID, NULL);
    }

    printf("customer count : %lu\n", customer_queue->count);
    // Kill any remaining customer processes
    for (int temp = 0; temp < customer_queue->count; temp++) {

        int i = (customer_queue->head + temp) % customer_queue->capacity;
        Customer *c = &((Customer*)customer_queue->elements)[i];

        printf("NOW KILLING CUSTOMER : %d\n", c->pid);
        if (c->pid > 0) {
            kill(c->pid, SIGINT);
        }
    }

    printf("cleaning up queue...\n");

    queueShmClear(customer_queue);
    cleanup_queue_shared_memory(customer_queue, shared_game->config.MAX_CUSTOMERS);

    // Clean up named semaphore
    if (complaint_sem != NULL) {
        sem_close(complaint_sem);
        sem_unlink(COMPLAINT_SEM_NAME);
    }

}

void handle_sigint(int signum) {
    exit(0);
}

// Handle customer messages
void handle_customer_message(int signum) {
    CustomerStatusMsg msg;

    // Try to receive messages from customers (non-blocking)
    while (msgrcv(msg_queue_id, &msg, sizeof(CustomerStatusMsg) - sizeof(long), 1, IPC_NOWAIT) != -1) {
        pid_t pid = msg.customer_pid;
        int cust_id = msg.customer_id;

        printf("Received message from customer %d (PID %d), action=%d, state=%d\n",
               cust_id, pid, msg.action, msg.state);

        // Find the customer in the queue
        int found = 0;
        for (int temp = 0; temp < customer_queue->count; temp++) {

            int i = (customer_queue->head + temp) % customer_queue->capacity;
            Customer *c = &((Customer*)customer_queue->elements)[i];
            if (c->pid == pid) {
                found = 1;

                // Process different action types
                switch (msg.action) {
                    case 0: // Status update
                        // Just update our knowledge of customer state
                        c->patience = msg.patience;
                        c->state = msg.state;
                        printf("Updated status for customer %d: patience=%.4f, state=%d\n",
                               cust_id, msg.patience, msg.state);
                        break;

                    case 1: // Normal leaving
                        printf("Customer %d is leaving normally\n", cust_id);
                        shared_game->num_customers_served++;
                        kill(c->pid, SIGINT);
                        queueShmRemoveAt(customer_queue, temp);
                        active_customers--;
                        break;

                    case 2: // Frustrated
                        printf("Customer %d left frustrated\n", cust_id);
                        shared_game->num_frustrated_customers++;
                        kill(c->pid, SIGINT);
                        queueShmRemoveAt(customer_queue, temp);
                        active_customers--;
                        break;

                    case 3: // Complained
                        printf("Customer %d complained and left\n", cust_id);

                        sem_wait(complaint_sem);
                        shared_game->num_complained_customers++;
                        shared_game->recent_complaint = true;
                        shared_game->complaining_customer_pid = c->pid;
                        shared_game->last_complaint_time = time(NULL);
                        sem_post(complaint_sem);
                        kill(c->pid, SIGINT);
                        queueShmRemoveAt(customer_queue, temp);
                        active_customers--;
                        break;

                    case 4: // Missing order
                        printf("Customer %d had missing order\n", cust_id);
                        shared_game->num_customers_missing++;
                        kill(c->pid, SIGINT);
                        queueShmRemoveAt(customer_queue, temp);
                        active_customers--;
                        break;

                    case 5: // Cascade effect
                        printf("Customer %d left due to cascade effect after seeing customer %d complain\n",
                              cust_id, shared_game->complaining_customer_pid);

                        sem_wait(complaint_sem);
                        shared_game->num_customers_cascade++;
                        sem_post(complaint_sem);
                        kill(c->pid, SIGINT);
                        queueShmRemoveAt(customer_queue, temp);
                        active_customers--;
                        break;
                }
                break;
            }
        }

        if (!found && msg.customer_pid > 0) {
//            kill(pid, SIGKILL); // Clean up the orphaned process
            printf("Unknown sender, killing process: %d\n", pid);
        }
    }
}


void check_and_reset_complaints() {
    sem_wait(complaint_sem);

    if (shared_game->recent_complaint) {
        time_t current_time = time(NULL);
        // Reset complaint after CASCADE_WINDOW seconds
        if (current_time - shared_game->last_complaint_time > shared_game->config.CASCADE_WINDOW) {
            shared_game->recent_complaint = false;
            printf("Complaint effect has expired\n");
        }
    }

    sem_post(complaint_sem);
}

void spawn_customer(int customer_id) {
    if (active_customers >= shared_game->config.MAX_CUSTOMERS) {
        return; // Don't spawn if we're at max capacity
    }

    Customer new_customer;
    // Create a new customer with random attributes
    create_random_customer(&new_customer, &shared_game->config);

    // Add to queue first (safer to do this before fork)
    if (queueShmEnqueue(customer_queue, &new_customer) == -1) {
        printf("Failed to add customer to queue\n");
        return;
    }

    size_t pos = (customer_queue->head + customer_queue->count - 1) % customer_queue->capacity;
    Customer *queue_customer = &((Customer*)customer_queue->elements)[pos];
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
        char customer_str[32];

        snprintf(msg_id_str, sizeof(msg_id_str), "%d", msg_queue_id);
        snprintf(cust_id_str, sizeof(cust_id_str), "%d", customer_id);
        serialize_customer(&new_customer, customer_str);

        execl("./customers", "./customers", msg_id_str, cust_id_str, customer_str, NULL);

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
    setup_shared_memory(&shared_game);
    max_customers = shared_game->config.MAX_CUSTOMERS;
    setup_queue_shared_memory(&customer_queue, shared_game->config.MAX_CUSTOMERS);
    initQueueShm(customer_queue, sizeof(Customer), shared_game->config.MAX_CUSTOMERS);

    // Initialize random number generator
    init_random();

    // Create named semaphore for complaint synchronization
    complaint_sem = sem_open(COMPLAINT_SEM_NAME, O_CREAT, 0666, 1);
    if (complaint_sem == SEM_FAILED) {
        perror("Failed to create complaint semaphore");
        exit(EXIT_FAILURE);
    }

    // Setup signal handler for customer messages
    signal(SIGUSR1, handle_customer_message);
    signal(SIGINT, handle_sigint);

    // Register cleanup handler
    atexit(cleanup_resources);

    // Create message queue for communication
    msg_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    printf("Customer Manager started. Message queue ID: %d\n", msg_queue_id);

    int next_customer_id = 0;

    // Main loop
    while (1) {
        // Spawn new customers if we're below max
        if (active_customers < shared_game->config.MAX_CUSTOMERS) {
            // Random chance to spawn a new customer
            if (random_float(0, 1) < shared_game->config.CUSTOMER_PROBABILITY) { // 30% chance each iteration
                spawn_customer(next_customer_id++);
            }
        }

        // Check and reset old complaints
        check_and_reset_complaints();

        // Process any pending messages (in case SIGUSR1 was missed)
        handle_customer_message(SIGUSR1);

        // Print statistics
        printf("Active: %d, Frustrated: %d, Complained: %d, Missing: %d, Cascade: %d\n",
               active_customers,
               shared_game->num_frustrated_customers,
               shared_game->num_complained_customers,
               shared_game->num_customers_missing,
               shared_game->num_customers_cascade);

        // Sleep for a random time between 1-3 seconds
        sleep(1);
    }

    return 0;
}