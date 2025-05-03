// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "products.h"



typedef struct {
    int MAX_TIME;
    int MAX_CUSTOMERS;
    float MAX_PATIENCE;
    float MIN_PATIENCE;
    float MAX_PATIENCE_DECAY;
    float MIN_PATIENCE_DECAY;
    int FRUSTRATED_CUSTOMERS;
    int COMPLAINED_CUSTOMERS;
    int CUSTOMERS_MISSING;
    float DAILY_PROFIT;
    int NUM_CHEFS;
    int NUM_BAKERS;
    int NUM_SELLERS;
    int NUM_SUPPLY_CHAIN;
    int MIN_PURCHASE_QUANTITY;
    int MAX_PURCHASE_QUANTITY;
    int MIN_TIME_FRUSTRATED;
    int MAX_TIME_FRUSTRATED;
    int MIN_OVEN_TIME;
    int MAX_OVEN_TIME;
    int NUM_OVENS;
    int MIN_BAKE_TIME;
    int MAX_BAKE_TIME;
    float CUSTOMER_PROBABILITY;
    int MIN_ORDER_ITEMS;
    int MAX_ORDER_ITEMS;
    float CUSTOMER_CASCADE_PROBABILITY;
    int CASCADE_WINDOW;
    int MIN_SELLER_PROCESSING_TIME;
    int MAX_SELLER_PROCESSING_TIME;
    int REALLOCATION_CHECK_INTERVAL;
    float PRODUCTION_RATIO_THRESHOLD;
    int MIN_CHEFS_PER_TEAM;
    int INGREDIENTS_TO_ORDER;
} Config;

int load_config(const char *filename, Config *config);
int load_product_catalog(const char *filename, ProductCatalog *catalog);
void print_config(Config *config);
int check_parameter_correctness(const Config *config);
void serialize_config(Config *config, char *buffer);
void deserialize_config(const char *buffer, Config *config);

#endif // CONFIG_H
