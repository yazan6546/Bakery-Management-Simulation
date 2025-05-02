#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>

#include "seller.h"
#include "bakery_message.h"
#include "customer.h"

// Get a descriptive string for seller state
const char* get_seller_state_string(SellerState state) {
    switch(state) {
        case IDLE: return "IDLE";
        case TAKING_ORDER: return "TAKING_ORDER";
        case PROCESSING_ORDER: return "PROCESSING_ORDER";
        case COMPLETING_ORDER: return "COMPLETING_ORDER";
        default: return "UNKNOWN";
    }
}

// Initialize a new seller with default values
void init_seller(Seller *seller, int id) {
    if (!seller) return;
    
    seller->id = id;
    seller->pid = getpid();
    seller->state = IDLE;
}

// Create a message queue or get existing one
int get_message_queue() {
    key_t key = ftok(".", 'M');  // Use current directory for key generation
    int msgid = msgget(key, 0666 | IPC_CREAT);
    
    if (msgid == -1) {
        perror("Failed to create/access message queue");
        return -1;
    }
    
    return msgid;
}

// Send a completion message to a customer
int send_completion_message(int msg_queue_id, pid_t customer_pid, float total_price, const char* status) {
    CompletionMessage msg;
    msg.mtype = customer_pid;  // Use customer's PID as message type
    strncpy(msg.status, status, sizeof(msg.status) - 1);
    msg.status[sizeof(msg.status) - 1] = '\0';  // Ensure null-termination
    msg.total_price = total_price;
    
    return msgsnd(msg_queue_id, &msg, sizeof(CompletionMessage) - sizeof(long), 0);
}