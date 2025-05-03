#ifndef BAKERY_MESSAGE_H
#define BAKERY_MESSAGE_H

#include "customer.h"
#define MAX_ITEM_NAME 50
#define MAX_TEAM_NAME 50

typedef struct {
    long mtype; // Message type: 1 (Bread), 2 (Cake/Sweets), 3 (Patisseries)
    char item_name[MAX_ITEM_NAME];
    ProductType category;
    int index;
} BakeryMessage;


// Message structure for both files
typedef struct {
    long mtype;           // Message type
    pid_t customer_pid;   // Customer identifier
    int customer_id;      // Customer ID for logging
    float patience;       // Current patience level
    CustomerState state;  // Current state
    int action;           // 0: status update, 1: leaving, 2: frustrated, 3: complained, 4: missing order
} CustomerStatusMsg;

#endif // BAKERY_MESSAGE_H
