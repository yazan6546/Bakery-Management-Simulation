//
// Created by yazan on 4/30/2025.
//

#include "customer.h"
#include "config.h"
#include "customer_utils.h"

#include "random.h"

Customer* create_random_customer(Config *config) {
    Customer *customer = (Customer *)malloc(sizeof(Customer));
    if (!customer) {
        perror("Failed to allocate memory for customer");
        return NULL;
    }
    customer->patience = random_float(config->MIN_PATIENCE, config->MAX_PATIENCE);
    customer->patience_decay -= random_float(config->MIN_PATIENCE_DECAY, config->MAX_PATIENCE_DECAY);
    customer->has_complained = false;
    customer->state = WALKING;
    return customer;
}

void serialize_customer(Customer *customer, char *buffer) {
    sprintf (buffer, "%f %f %d %d",
            customer->patience,
            customer->patience_decay,
            customer->has_complained,
            customer->state);
}

void deserialize_customer(Customer *customer, char *buffer) {
    sscanf (buffer, "%f %f %d %d",
            &customer->patience,
            &customer->patience_decay,
            &customer->has_complained,
            &customer->state);
}
void free_customer(Customer *customer) {
    if (customer) {
        free(customer);
    }
}
