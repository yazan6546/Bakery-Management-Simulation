//
// Created by yazan on 4/26/2025.
//

#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include "products.h"




// Inventory struct with array-based approach
typedef struct {
    int quantities[NUM_INGREDIENTS];  // Array of ingredient quantities
    int max_capacity;
} Inventory;

// In inventory.h
typedef struct {
    int quantities[MAX_PRODUCTS_PER_CATEGORY];  // Quantities for specific products within category
    int product_count;                          // Number of product types in this category
} ReadyProductCategory;

typedef struct {
    ReadyProductCategory categories[NUM_PRODUCTS];  // Array indexed by ProductType enum
    int total_count;                               // Total number of products ready
    int max_capacity;                              // Maximum storage capacity
} ReadyProducts;

// Function prototypes for inventory operations
void init_inventory(Inventory *inventory);
void add_ingredient(Inventory *inventory, IngredientType type, int quantity, sem_t* sem);
void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS], sem_t* sem);
int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS], sem_t* sem);
void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS], sem_t* sem);
void restock_ingredients(Inventory *inventory, sem_t* sem);
print_inventory(Inventory *inventory);



void init_ready_products(ReadyProducts *ready_products);
void add_ready_product(ReadyProducts *ready_products, ProductType type, int product_index, int quantity, sem_t* sem);
int get_ready_product(ReadyProducts *ready_products, ProductType type, int product_index, int quantity, sem_t* sem);


#endif //INVENTORY_H