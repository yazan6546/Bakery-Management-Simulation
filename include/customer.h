#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <sys/types.h>
#include <stdbool.h>

#define MAX_NAME_LEN 32

// Feedback values
#define FEEDBACK_GOOD 1
#define FEEDBACK_BAD 2

// Customer states
typedef enum {
    WALKING,
    WAITING_IN_QUEUE,
    ORDERING,
    WAITING_FOR_ORDER,
    COMPLAINING,
    LEFT
} CustomerState;

typedef struct {
    int id;
    int patience;  // in seconds
    int has_complained;
    CustomerState state;
} Customer;

#endif // CUSTOMER_H