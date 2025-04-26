// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    int FRUSTRATED_CUSTOMERS;
    int COMPLAINED_CUSTOMERS;
    int CUSTOMERS_MISSING;
    float DAILY_PROFIT;
    int NUM_BREAD_CATEGORIES;
    int NUM_SANDWICH_CATEGORIES;
    int NUM_CAKE_FLAVORS;
    int NUM_SWEET_CATEGORIES;
    int NUM_SWEET_FLAVORS;



} Config;

int load_config(const char *filename, Config *config);
void print_config(Config *config);
int check_parameter_correctness(const Config *config);
void serialize_config(Config *config, char *buffer);
void deserialize_config(Config *config, char *buffer);
#endif // CONFIG_H
