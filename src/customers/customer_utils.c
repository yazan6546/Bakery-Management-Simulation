//
// Created by yazan on 4/30/2025.
//

#include "customer.h"
#include "config.h"
#include "customer_utils.h"

Customer* create_random_customer(Config *config) {
    Customer *customer = (Customer *)malloc(sizeof(Customer));
    if (!customer) {
        perror("Failed to allocate memory for customer");
        return NULL;
    }
    customer->patience = config.;
    customer->has_complained = 0;
    customer->state = WALKING;
    return customer;
}