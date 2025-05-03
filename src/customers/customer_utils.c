//
// Created by yazan on 4/30/2025.
//

#include "customer.h"
#include "config.h"
#include "game.h"
#include "random.h"
#include "queue.h"

void create_random_customer(Customer *customer, Config *config) {

    if (!customer)
        return;
    customer->patience = random_float(config->MIN_PATIENCE, config->MAX_PATIENCE);
    customer->patience_decay = random_float(config->MIN_PATIENCE_DECAY, config->MAX_PATIENCE_DECAY);
    customer->has_complained = false;
    customer->state = WALKING;
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

void print_customer(Customer *customer) {
    printf("Customer: %f %f %d %d\n", customer->patience, customer->patience_decay, customer->has_complained, customer->state);

}


void generate_random_customer_order(CustomerOrder *order, Game *game) {

    order->item_count = 0;
    order->total_price = 0;

    int num_items = (int) random_float(game->config.MIN_ORDER_ITEMS, game->config.MAX_ORDER_ITEMS);  // Order 1-3 items

    // Generate each item in the order
    for (int i = 0; i < num_items && i < game->config.MAX_ORDER_ITEMS; i++) {
        // Get the product catalog
        ProductCatalog *catalog = &game->productCatalog;

        // Pick a random category with products
        int attempts = 0;
        ProductCategory *category;
        do {

            // Pick a random category
            int random_category = rand() % catalog->category_count;
            category = &catalog->categories[random_category];
            if (++attempts > catalog->category_count * 2) {
                break;  // Avoid infinite loop
            }
        } while (category->product_count <= 0);

        if (category->product_count <= 0) {
            continue;  // Skip if no products in category
        }

        // Pick a random product from the category
        int random_product = rand() % category->product_count;

        // Add to order with a quantity between 1-3
        order->items[order->item_count].product = category->products[random_product];
        order->items[order->item_count].quantity = (int) random_float(game->config.MIN_PURCHASE_QUANTITY,
                                                              game->config.MAX_PURCHASE_QUANTITY);

        order->items[order->item_count].type = category->type;
        order->items[order->item_count].product_index = random_product;

        // Calculate price for this item
        float item_price = order->items[order->item_count].product.price *
                           order->items[order->item_count].quantity;
        order->total_price += item_price;

        order->item_count++;
    }
}