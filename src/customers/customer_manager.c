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


int find_and_update_customer(pid_t pid, queue_shm *customer_queue, sem_t *queue_sem,
    CustomerState new_state, float new_patience);
void handle_customer_state(CustomerStatusMsg msg);
int find_and_remove_customer(pid_t pid, queue_shm *customer_queue, sem_t *queue_sem);

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

void process_customer_messages(int msg_queue_id, queue_shm *customer_queue, Game *shared_game, sem_t *queue_sem) {
    CustomerStatusMsg msg;

    while (msgrcv(msg_queue_id, &msg, sizeof(CustomerStatusMsg) - sizeof(long), 1, IPC_NOWAIT) != -1) {
        pid_t pid = msg.customer_pid;

        switch (msg.action) {
            case STATUS_UPDATE: {

                find_and_update_customer(pid, customer_queue, queue_sem, msg.state, msg.patience);
                break;
            }

            case LEAVING_NORMALLY: {

                active_customers--;
                shared_game->num_customers_served++;
                kill(SIGINT, pid);
                if (msg.in_queue) {
                    find_and_remove_customer(msg.customer_pid, customer_queue, queue_sem);
                }
                break;
            }
            case LEAVING_EARLY: {
                active_customers--;
                kill(SIGINT, pid);
                if (msg.in_queue) {
                    find_and_remove_customer(msg.customer_pid, customer_queue, queue_sem);
                }
                handle_customer_state(msg);
            }
                break;

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

    sem_t *queue_sem = sem_open(QUEUE_SEM_NAME, O_CREAT, 0666, 1);
    if (queue_sem == SEM_FAILED) {
        perror("Failed to create queue semaphore");
        exit(EXIT_FAILURE);
    }

    // Setup signal handler for customer messages
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

        process_customer_messages(msg_queue_id, customer_queue, shared_game, queue_sem);

        for (int i = 0; i < shared_game->config.MAX_CUSTOMERS; i++) {
            size_t index = (customer_queue->head + i) % customer_queue->capacity;
            Customer *c = &((Customer*)customer_queue->elements)[index];

            if (c->pid > 0) {
                printf("Customer %d (PID: %d) is in queue with state: %d\n", c->id, c->pid, c->state);
            }
        }

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

int find_and_update_customer(pid_t pid, queue_shm *customer_queue, sem_t *queue_sem, CustomerState new_state, float new_patience) {
    int found = -1;

    // Lock the queue with semaphore for thread safety
    sem_wait(queue_sem);

    // Find the customer in the queue
    for (int i = 0; i < customer_queue->count; i++) {
        size_t index = (customer_queue->head + i) % customer_queue->capacity;
        Customer *c = &((Customer*)customer_queue->elements)[index];

        if (c->pid == pid) {
            // Update customer data in place
            c->state = new_state;
            c->patience = new_patience;

            printf("Updated customer %d in queue: state=%d, patience=%.2f\n",
                  c->id, c->state, c->patience);

            found = i;
            break;
        }
    }

    // Release the semaphore
    sem_post(queue_sem);

    return found;  // Returns -1 if not found, or the index if found
}

int find_and_remove_customer(pid_t pid, queue_shm *customer_queue, sem_t *queue_sem) {
    int found = -1;

    // Lock the queue for the entire operation
    sem_wait(queue_sem);

    // Search for the customer in the queue
    for (size_t i = 0; i < customer_queue->count; i++) {
        size_t index = (customer_queue->head + i) % customer_queue->capacity;
        Customer *c = &((Customer*)customer_queue->elements)[index];

        if (c->pid == pid) {
            // Found the customer, remove at this position
            found = i;

            // Remove customer at the found index
            if (queueShmRemoveAt(customer_queue, i) != 0) {
                // If removal failed, set found back to -1
                found = -1;
            }
            break;
        }
    }

    // Unlock the queue
    sem_post(queue_sem);

    return found;  // Returns -1 if not found or removal failed, or the index if found and removed
}

void handle_customer_state(CustomerStatusMsg msg) {
    switch (msg.state) {
        case FRUSTRATED:
            shared_game->num_frustrated_customers++;
            break;

        case COMPLAINING: {
            shared_game->num_complained_customers++;
            shared_game->complaining_customer_pid = msg.customer_pid;
            shared_game->last_complaint_time = time(NULL);
            shared_game->recent_complaint = true;
            break;
        }
        case MISSING_ORDER:
            shared_game->num_customers_missing++;
            break;
        case CONTAGION:
            shared_game->num_customers_cascade++;
            break;
    }
}