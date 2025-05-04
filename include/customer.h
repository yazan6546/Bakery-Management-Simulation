#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <sys/types.h>
#include <stdbool.h>
#include "queue.h"

#define MAX_NAME_LEN 32

// Feedback values
#define FEEDBACK_GOOD 1
#define FEEDBACK_BAD 2
// Named semaphore for complaint synchronization

#include "config.h"
#include "game.h"
// Customer states
typedef enum {
    WALKING,
    WAITING_IN_QUEUE,
    ORDERING,
    WAITING_FOR_ORDER,
    FRUSTRATED,
    COMPLAINING,
    MISSING_ORDER,
    CONTAGION
} CustomerState;

typedef struct {
    int id;
    pid_t pid;
    float patience;  // in seconds
    float patience_decay;  // in seconds
    bool has_complained;
    short seller_id;
    CustomerState state;
} Customer;


void create_random_customer(Customer *customer, Config *config);
void deserialize_customer(Customer *customer, char *buffer);
void serialize_customer(Customer *customer, char *buffer);
void free_customer(Customer *customer);
void print_customer(Customer *customer);
void generate_random_customer_order(CustomerOrder *order, Game *game);
void cleanup_queue_shared_memory(queue_shm *queue_shm, size_t capacity);
#endif // CUSTOMER_H