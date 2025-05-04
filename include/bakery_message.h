#ifndef BAKERY_MESSAGE_H
#define BAKERY_MESSAGE_H

#include "customer.h"
#include "chef.h"
#define MAX_ITEM_NAME 50
#define MAX_TEAM_NAME 50
#define MAX_NAME_LENGTH 64

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

// Add to bakery_message.h
typedef struct {
    long mtype;                    // Message type
    CustomerOrder order;           // Order details
} OrderMessage;

typedef enum {
    ORDER_SUCCESS,
    ORDER_FAILED
} OrderResult;

typedef struct {
    long mtype;           // Message type (should match customer PID)
    OrderResult result;   // Success or failure
    float total_price;    // Total price of completed order (0 if failed)
} CompletionMessage;


int get_message_queue(void);


typedef struct {
    long mtype;
    ChefTeam source_team;
    char product_name[MAX_NAME_LENGTH];
    int product_index;
} ChefMessage;

// Message structure for restock requests
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int urgency; // 0-10 scale, 10 being most urgent
} RestockRequest;


// Message structure for restock confirmations
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int success; // 1 if successful, 0 if failed
} RestockConfirmation;


#endif // BAKERY_MESSAGE_H
