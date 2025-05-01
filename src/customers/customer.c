#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include "customer.h"
#include "game.h"
#include "random.h"

// Global variables
int customer_id;
pid_t my_pid;
int msg_queue_id;
Game *shared_game;
queue_shm *customer_queue;
Customer *my_entry;  // Direct pointer to our entry in shared memory

// Simplified message structure - just for leave notifications
typedef struct {
    long mtype;
    pid_t customer_pid;
} LeaveMessage;

// Update customer state directly in shared memory
void update_state(CustomerState new_state) {
    // Directly update our entry in shared memory - no locking needed!
    my_entry->state = new_state;
    printf("Customer %d updated state to %d\n", customer_id, new_state);
}

// Update patience directly in shared memory
void update_patience(float new_patience) {
    my_entry->patience = new_patience;
}

// Notify manager and exit
void leave_restaurant(CustomerState final_state) {

    // Send leave notification to manager
    LeaveMessage msg;
    msg.mtype = 1;  // To manager
    msg.customer_pid = my_pid;

    if (msgsnd(msg_queue_id, &msg, sizeof(pid_t), 0) == -1) {
        perror("Failed to send leave message");
    }

    // Notify manager to check message queue
    kill(getppid(), SIGUSR1);

    exit(EXIT_SUCCESS);
}

// Handle patience decay
void handle_alarm(int sig) {
    // Only decay patience if not ordering
    if (my_entry->state != ORDERING) {
        float new_patience = my_entry->patience - my_entry->patience_decay;
        update_patience(new_patience);

        printf("Customer %d patience: %.1f\n", customer_id, new_patience);

        if (new_patience <= 0) {
            printf("Customer %d ran out of patience and is leaving\n", customer_id);
            // Update game stats
            shared_game->num_frustrated_customers++;
            leave_restaurant(FRUSTRATED);
        }
    }

    // Set next alarm
    alarm(1);
}

// Handle signals from seller
void handle_seller_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Customer %d: It's my turn to order!\n", customer_id);
        update_state(ORDERING);

        // Reset patience when it's our turn
        update_patience(shared_game->config.MAX_PATIENCE);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <msg_queue_id> <customer_id> <queue_offset>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("oifewiojfweoifjwen\n");

    // Parse arguments
    msg_queue_id = atoi(argv[1]);
    customer_id = atoi(argv[2]);
    size_t queue_offset = atoi(argv[3]);
    my_pid = getpid();

    // Initialize random number generator
    srand(time(NULL) ^ my_pid);

    // Attach to game shared memory
    int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed for game");
        exit(EXIT_FAILURE);
    }
    shared_game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    fcntl(shm_fd, F_SETFD, fcntl(shm_fd, F_GETFD) & ~FD_CLOEXEC);

    close(shm_fd);

    // Attach to queue shared memory
    int queue_fd = shm_open("/customer_queue_shm", O_RDWR, 0666);
    if (queue_fd == -1) {
        perror("shm_open failed for queue");
        exit(EXIT_FAILURE);
    }

    size_t queue_size = queueShmSize(sizeof(Customer), shared_game->config.MAX_CUSTOMERS);
    void* queue_ptr = mmap(NULL, queue_size, PROT_READ | PROT_WRITE, MAP_SHARED, queue_fd, 0);
    customer_queue = (queue_shm*)queue_ptr;
    close(queue_fd);

    print_config(&shared_game->config);

    // Get direct pointer to our entry in the queue
    my_entry = (Customer*)(customer_queue->elements + queue_offset);

    // Set up signal handlers
    signal(SIGALRM, handle_alarm);
    signal(SIGUSR1, handle_seller_signal);

    printf("Customer %d created with patience %.1f, decay %.1f\n",
           customer_id, my_entry->patience, my_entry->patience_decay);

    // Start patience decay timer
    alarm(1);

    // Customer state machine
    while (1) {
        switch (my_entry->state) {
            case WALKING:
                printf("Customer %d is walking...\n", customer_id);
                sleep(1 + rand() % 3);
                update_state(WAITING_IN_QUEUE);
                break;

            case WAITING_IN_QUEUE:
                printf("Customer %d is waiting in queue...\n", customer_id);
                sleep(2);
                break;

            case ORDERING:
                printf("Customer %d is ordering...\n", customer_id);
                sleep(2);
                update_state(WAITING_FOR_ORDER);
                break;

            case WAITING_FOR_ORDER:
                printf("Customer %d is waiting for order...\n", customer_id);
                sleep(3);

                // 10% chance of missing order
                if (random_float(0, 1) < 0.1) {
                    printf("Customer %d: Order is missing!\n", customer_id);
                    shared_game->num_customers_missing++;
                    leave_restaurant(WAITING_FOR_ORDER);
                }

                // 20% chance of complaining
                if (random_float(0, 1) < 0.2) {
                    printf("Customer %d is complaining about the order\n", customer_id);
                    my_entry->has_complained = true;
                    shared_game->num_complained_customers++;
                    leave_restaurant(COMPLAINING);
                }

                // Happy customer
                printf("Customer %d is happy with the order and leaves\n", customer_id);
                leave_restaurant(WAITING_FOR_ORDER);
                break;

            case FRUSTRATED:
                leave_restaurant(FRUSTRATED);
                break;

            case COMPLAINING:
                leave_restaurant(COMPLAINING);
                break;

            default:
                break;
        }

        sleep(1);
    }

    return 0;
}