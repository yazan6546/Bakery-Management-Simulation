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