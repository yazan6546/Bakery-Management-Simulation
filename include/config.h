// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "products.h"



typedef struct {
    int MAX_TIME;
    int FRUSTRATED_CUSTOMERS;
    int COMPLAINED_CUSTOMERS;
    int CUSTOMERS_MISSING;
    float DAILY_PROFIT;
    int NUM_BREAD_CATEGORIES;
    int NUM_SANDWICH_CATEGORIES;
    int NUM_CAKE_FLAVORS;
    int NUM_SWEET_CATEGORIES;
    int NUM_SWEET_FLAVORS;
    int NUM_SAVORY_PATISSERIES;
    int NUM_SWEET_PATISSERIES;
    int NUM_CHEFS;
    int NUM_BAKERS;
    int NUM_SELLERS;
    int NUM_SUPPLY_CHAIN;
    int NUM_PASTRY_CATEGORIES;
    int MIN_PURCHASE_QUANTITY;
    int MAX_PURCHASE_QUANTITY;
    float MIN_ITEM_PRICE;
    float MAX_ITEM_PRICE;
    int MIN_TIME_FRUSTRATED;
    int MAX_TIME_FRUSTRATED;
    int MIN_OVEN_TIME;
    int MAX_OVEN_TIME;
    int NUM_OVENS;
    int MIN_BAKE_TIME;
    int MAX_BAKE_TIME;
} Config;

int load_config(const char *filename, Config *config);
void print_config(Config *config);
int check_parameter_correctness(const Config *config);
void serialize_config(Config *config, char *buffer);
void deserialize_config(const char *buffer, Config *config);
ProductType get_product_type_from_string(const char* name);
IngredientType get_ingredient_type_from_string(const char* name);

#endif // CONFIG_H
