#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <sys/types.h>
#include <stdbool.h>

#define MAX_NAME_LEN 32

// Feedback values
#define FEEDBACK_GOOD 1
#define FEEDBACK_BAD 2

#include "config.h"
#include "game.h"
#include "queue.h"
// Customer states
typedef enum {
    WALKING,
    WAITING_IN_QUEUE,
    WAITING_FOR_ORDER,
    ORDERING,
    FRUSTRATED,
    COMPLAINING
} CustomerState;

typedef struct {
    pid_t pid;
    float patience;  // in seconds
    float patience_decay;  // in seconds
    bool has_complained;
    CustomerState state;
} Customer;


void create_random_customer(Customer *customer, Config *config);
void deserialize_customer(Customer *customer, char *buffer);
void serialize_customer(Customer *customer, char *buffer);
void free_customer(Customer *customer);
void setup_shared_memory(queue_shm **customer_queue, Game **shared_game);
#endif // CUSTOMER_H