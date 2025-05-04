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
#include "semaphores_utils.h"
#include "shared_mem_utils.h"

// Global variables
int customer_id;
pid_t my_pid;
int msg_queue_id;
float original_patience;
Game *shared_game;
Customer my_entry;
sem_t *complaint_sem;
volatile sig_atomic_t in_queue = 1;

void handle_state(CustomerState state, Game *shared_game, int gloabl_msg);
void handle_seller_signal(int sig);
void send_status_message(int action_type, bool in_queue);
void update_state(CustomerState new_state, bool in_queue);
void update_patience(float new_patience);
void leave_restaurant(CustomerState final_state, int action_type, bool in_queue);
void handle_alarm(int sig);
void handle_sigint_customer(int sig);
void check_for_contagion(Game *shared_game);
void send_order_message(int msg_queue_id, CustomerOrder *order);
void setup_sigint_handler();
// Send status update to manager

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <msg_queue_id> <customer_id> <customer_buffer>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setup_sigint_handler();

    int global_msg = get_message_queue();
    setup_shared_memory(&shared_game);
    complaint_sem = sem_open(COMPLAINT_SEM_NAME, O_CREAT, 0666, 1);
    if (complaint_sem == SEM_FAILED) {
        perror("Failed to open complaint semaphore");
        exit(EXIT_FAILURE);
    }
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
    signal(SIGINT, handle_sigint_customer);

    // Initial status notification
    send_status_message(0, in_queue); // 0 = status update

    alarm(1);  // Start the timer
    // Customer state machine
    while (1) {
        printf("Customer %d patience : %.4f\n", customer_id, my_entry.patience);
        handle_state(my_entry.state, shared_game, global_msg);

        if (my_entry.state != WAITING_FOR_ORDER) {
            pause();
        }
    }

    return 0;
}

void handle_state(CustomerState state, Game *shared_game, int gloabl_msg) {

    // Check for cascade effect in most states
    if (state != COMPLAINING && state != FRUSTRATED && state != CONTAGION) {
        check_for_contagion(shared_game);
    }

    switch (state) {
        case WALKING:
            printf("Customer %d is walking...\n", customer_id);
            sleep(1 + rand() % 3);
            update_state(WAITING_IN_QUEUE, in_queue);
            break;

        case WAITING_IN_QUEUE:
            printf("Customer %d is waiting in queue...\n", customer_id);
            pause(); // pause until seller signals
            break;

        case ORDERING:
            alarm(0); // Stop the timer
            printf("Customer %d is ordering...\n", customer_id);
            sleep(2); // simulate ordering time
            CustomerOrder order;
            generate_random_customer_order(&order, shared_game);
            send_order_message(gloabl_msg, &order); // send order to seller
            update_state(WAITING_FOR_ORDER, in_queue);
            break;

        case WAITING_FOR_ORDER:
            printf("Customer %d is waiting for order...\n", customer_id);
            fflush(stdout);

            CompletionMessage completion_msg;
            // Remove IPC_NOWAIT to make the call blocking
            if (msgrcv(gloabl_msg, &completion_msg, sizeof(CompletionMessage) - sizeof(long), my_pid, 0) == -1) {
                perror("Error receiving order completion");
                leave_restaurant(FRUSTRATED, LEAVING_EARLY, in_queue); // 2 = FRUSTRATED
            }

            if (completion_msg.result == ORDER_SUCCESS) {
                printf("Customer %d received order successfully, total price: %.2f\n", customer_id, completion_msg.total_price);
                leave_restaurant(WAITING_FOR_ORDER, LEAVING_NORMALLY, in_queue); // 1 = normal leaving
            } else if (completion_msg.result == ORDER_MISSING) {
                printf("Customer %d's order failed!\n", customer_id);
                leave_restaurant(MISSING_ORDER, LEAVING_EARLY, in_queue); // 4 = missing order
            }
            else if (completion_msg.result == ORDER_FAILED) {
                printf("Customer %d's order failed!\n", customer_id);
                leave_restaurant(ORDER_MISSING, LEAVING_EARLY, in_queue); // 2 = frustrated
            }
            break;

        case FRUSTRATED:
            leave_restaurant(FRUSTRATED, LEAVING_EARLY, in_queue); // 2 = frustrated
            pause();  // wait for manager to handle
            break;

        case MISSING_ORDER:
            printf("Customer %d is missing order\n", customer_id);
            leave_restaurant(MISSING_ORDER, LEAVING_EARLY, in_queue); // 4 = missing order
            break;

        case COMPLAINING:
            leave_restaurant(COMPLAINING, LEAVING_EARLY, in_queue); // 3 = complained
            pause(); // wait for manager to handle
            break;

        case CONTAGION:
            printf("Customer %d is leaving due to contagion\n", customer_id);
            leave_restaurant(CONTAGION, LEAVING_EARLY, in_queue); // 5 = cascade effect
            break;

        default:
            break;
    }
}


void send_status_message(int action_type, bool in_queue) {
    CustomerStatusMsg msg;
    msg.mtype = 1;  // To manager
    msg.customer_pid = my_pid;
    msg.customer_id = customer_id;
    msg.in_queue = in_queue;
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
void update_state(CustomerState new_state, bool in_queue) {
    my_entry.state = new_state;
    printf("Customer %d updated state to %d\n", customer_id, new_state);

    // Send status update after state change
    send_status_message(STATUS_UPDATE, in_queue);
}

// Update patience directly in shared memory
void update_patience(float new_patience) {
    my_entry.patience = new_patience;
    // No need to notify manager for every patience update
}

// Notify manager and exit
void leave_restaurant(CustomerState final_state, int action_type, bool in_queue) {
    my_entry.state = final_state;
    send_status_message(action_type, in_queue);
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
            send_status_message(0, in_queue);
        }

        if (my_entry.patience <= 0) {
            alarm(0); // Stop the timer
            printf("Customer %d ran out of patience and is leaving\n", customer_id);
            // Let manager update game stats
            update_state(FRUSTRATED, in_queue);
            return;
        }
    }

    // Set next alarm
    alarm(1);
}

// Handle signals from seller
void handle_seller_signal(int sig) {

    printf("me  order qe2opek\n");
    if (sig == SIGUSR1) {
        in_queue = 0;
        update_state(ORDERING, in_queue); // Update state to ORDERING

        // Reset patience when it's our turn
        update_patience(original_patience);
    }
}

void handle_sigint_customer(int sig) {

    // Block all signals during handler execution
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    alarm(0);


    if (shared_game != NULL)
        munmap(shared_game, sizeof(Game));

    _exit(EXIT_SUCCESS);
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

        leave_restaurant(CONTAGION, 5, in_queue); // 5 = cascade effect
    }
}

void send_order_message(int msg_queue_id, CustomerOrder *order) {
    OrderMessage order_msg;
    order_msg.mtype = getpid();  // Use customer's PID as message type
    order_msg.order = *order;

    printf("stuck here/?\n");
    if (msgsnd(msg_queue_id, &order_msg, sizeof(OrderMessage) - sizeof(long), 0) == -1) {
        perror("Failed to send order message");
        leave_restaurant(FRUSTRATED, 2, in_queue); // 2 = FRUSTRATED
    }
    printf("Customer %d sent order message to seller\n", customer_id);
}



// Setup SIGINT handler function - call this in your main
void setup_sigint_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_sigint_customer;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGALRM); // Block SIGALRM during handler
    sa.sa_flags = 0; // Don't restart system calls

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    // Ensure SIGINT isn't blocked
    sigset_t unblock;
    sigemptyset(&unblock);
    sigaddset(&unblock, SIGINT);
    sigprocmask(SIG_UNBLOCK, &unblock, NULL);
}

