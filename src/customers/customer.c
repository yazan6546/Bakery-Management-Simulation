#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include "bakery_message.h"
#include <signal.h>
#include "customer.h"
#include "products.h"
#include "random.h"
#include "time.h"

// Global variables
int customer_id;
pid_t my_pid;
int msg_queue_id;
float original_patience;
Customer my_entry;
sem_t *complaint_sem;

void handle_state(CustomerState state, Game *shared_game);
void handle_seller_signal(int sig);
void send_status_message(int action_type);
void update_state(CustomerState new_state);
void update_patience(float new_patience);
void leave_restaurant(CustomerState final_state, int action_type);
void handle_alarm(int sig);
void handle_sigint(int sig);
void check_for_contagion(Game *shared_game);

// Send status update to manager

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <msg_queue_id> <customer_id> <customer_buffer>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Setup game shared memory as before
    int shm_fd = shm_open("/game_shared_mem", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Game));
    Game *shared_game = mmap(0, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_game == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }

    complaint_sem = sem_open(COMPLAINT_SEM_NAME, O_CREAT, 0666, 1);
    if (complaint_sem == SEM_FAILED) {
        perror("Failed to open complaint semaphore");
        exit(EXIT_FAILURE);
    }

    fcntl(shm_fd, F_SETFD, fcntl(shm_fd, F_GETFD) & ~FD_CLOEXEC);

    close(shm_fd);
    // Initialize game state

    // Parse arguments
    msg_queue_id = atoi(argv[1]);
    customer_id = atoi(argv[2]);
    char *customer_buffer = argv[3];

    deserialize_customer(&my_entry, customer_buffer);
    original_patience = my_entry.patience;
    my_pid = getpid();

    // Initialize random number generator
    init_random();

    // Set up signal handlers
    signal(SIGALRM, handle_alarm);
    signal(SIGUSR1, handle_seller_signal);

    // Initial status notification
    send_status_message(0);

    // Start patience decay timer
    alarm(1);

    // Customer state machine
    while (1) {
        printf("Customer %d patience : %.4f\n", customer_id, my_entry.patience);
        handle_state(my_entry.state, shared_game);
        pause();
    }

    return 0;
}

void handle_state(CustomerState state, Game *shared_game) {

    // Check for cascade effect in most states
    if (state != COMPLAINING && state != FRUSTRATED && state != CONTAGION) {
        check_for_contagion(shared_game);
    }

    switch (state) {
        case WALKING:
            printf("Customer %d is walking...\n", customer_id);
            sleep(1 + rand() % 3);
            update_state(WAITING_IN_QUEUE);
            break;

        case WAITING_IN_QUEUE:
            printf("Customer %d is waiting in queue...\n", customer_id);
            sleep(2);
            // pause until seller signals
            break;

        case ORDERING:
            printf("Customer %d is ordering...\n", customer_id);
            sleep(2);
            CustomerOrder order;
            generate_random_customer_order(&order, shared_game);
            update_state(WAITING_FOR_ORDER);
            break;

        case WAITING_FOR_ORDER:
            printf("Customer %d is waiting for order...\n", customer_id);
            sleep(3);
            // send to seller here...
            // 10% chance of missing order
            if (random_float(0, 1) < 0.1) {
                printf("Customer %d: Order is missing!\n", customer_id);
                leave_restaurant(WAITING_FOR_ORDER, 4); // 4 = missing order
                break;
            }

            // 20% chance of complaining
            if (random_float(0, 1) < 0.2) {
                printf("Customer %d is complaining about the order\n", customer_id);
                my_entry.has_complained = true;
                leave_restaurant(COMPLAINING, 3); // 3 = complained
                break;
            }

            // Happy customer
            printf("Customer %d is happy with the order and leaves\n", customer_id);
            leave_restaurant(WAITING_FOR_ORDER, 1); // 1 = normal leaving
            break;

        case FRUSTRATED:
            leave_restaurant(FRUSTRATED, 2); // 2 = frustrated
            pause();  // wait for manager to handle
            break;

        case COMPLAINING:
            leave_restaurant(COMPLAINING, 3); // 3 = complained
            pause(); // wait for manager to handle
            break;

        default:
            break;
    }
}


void send_status_message(int action_type) {
    CustomerStatusMsg msg;
    msg.mtype = 1;  // To manager
    msg.customer_pid = my_pid;
    msg.customer_id = customer_id;
    msg.patience = my_entry.patience;
    msg.state = my_entry.state;
    msg.action = action_type;

    if (msgsnd(msg_queue_id, &msg, sizeof(CustomerStatusMsg) - sizeof(long), 0) == -1) {
        perror("Failed to send status message");
    }

    // Signal manager to check message queue
//    kill(getppid(), SIGUSR1);
}

// Update customer state directly in shared memory
void update_state(CustomerState new_state) {
    my_entry.state = new_state;
    printf("Customer %d updated state to %d\n", customer_id, new_state);

    // Send status update after state change
    send_status_message(0);
}

// Update patience directly in shared memory
void update_patience(float new_patience) {
    my_entry.patience = new_patience;
    // No need to notify manager for every patience update
}

// Notify manager and exit
void leave_restaurant(CustomerState final_state, int action_type) {
    update_state(final_state);
    send_status_message(action_type);
    exit(EXIT_SUCCESS);
}

// Handle patience decay
void handle_alarm(int sig) {
    // Only decay patience if not ordering
    if (my_entry.state != ORDERING) {
        my_entry.patience -= my_entry.patience_decay;

        // Send periodic patience updates (every 3 seconds to avoid flooding)
        static int update_counter = 0;
        if (++update_counter % 2 == 0) {
            send_status_message(0);
        }

        if (my_entry.patience <= 0) {
            printf("Customer %d ran out of patience and is leaving\n", customer_id);
            // Let manager update game stats
            update_state(FRUSTRATED);
            alarm(0); // Stop the timer
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
        update_patience(original_patience);
    }
}

void handle_sigint(int sig) {
    printf("Customer %d received SIGINT, exiting...\n", customer_id);
    shm_unlink("/game_shared_mem");
    exit(EXIT_SUCCESS);
}


void check_for_contagion(Game *shared_game) {

    // Wait for semaphore before reading complaint data
    sem_wait(complaint_sem);

    // Read complaint data atomically
    bool has_complaint = shared_game->recent_complaint;
    pid_t complaining_pid = shared_game->complaining_customer_pid;

    // Release semaphore
    sem_post(complaint_sem);

    // Skip if no complaints or we're the one complaining
    if (!has_complaint || complaining_pid == my_pid) {
        return;
    }

    // Check if the complaint is recent (within configured window)
    float cascade_prob = shared_game->config.CUSTOMER_CASCADE_PROBABILITY;
    if (random_float(0, 1) < cascade_prob) {printf("Customer %d saw customer %d complaining and decided to leave too!\n",
                   customer_id, complaining_pid);

        leave_restaurant(CONTAGION, 5); // 5 = cascade effect
    }
}


