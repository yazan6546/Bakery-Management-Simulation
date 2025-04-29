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
#include "game.h"

// Define names for shared memory file and semaphore

#define SEM_NAME "/bakery_inventory_sem"
#define READY_SEM_NAME "/bakery_ready_products_sem"

// Define enum for ingredient types
typedef enum {
    WHEAT,
    YEAST,
    BUTTER,
    MILK,
    SUGAR,
    SALT,
    SWEET_ITEMS,
    CHEESE,
    SALAMI,
    PASTE_INGREDIENTS,
    NUM_INGREDIENTS  // This will automatically equal the number of ingredients
} IngredientType;

typedef enum {
    BREAD,
    CAKE,
    SANDWICH,
    SWEET,
    SWEET_PATISSERIES,
    SAVORY_PATISSERIES,
    NUM_PRODUCTS  // This will automatically equal the number of products
} ProductType;

// Inventory struct with array-based approach
typedef struct {
    int quantities[NUM_INGREDIENTS];  // Array of ingredient quantities
    int max_capacity;
} Inventory;

// Ready products struct to store finished products
typedef struct {
    int quantities[NUM_PRODUCTS];  // Array of product quantities
    int max_capacity;
} ReadyProducts;

// Shared memory globals
extern sem_t* inventory_sem;
extern sem_t* ready_products_sem;

// Function prototypes for inventory operations
void init_inventory(Inventory *inventory);
void add_ingredient(Inventory *inventory, IngredientType type, int quantity);
void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
void restock_ingredients(Inventory *inventory);

// Function prototypes for shared memory and semaphores
int setup_inventory_semaphore(void);
void lock_inventory(void);
void unlock_inventory(void);
void cleanup_resources(void);

// Function prototypes for ready products
int setup_ready_products_semaphore(void);
void init_ready_products(ReadyProducts *ready_products);
void add_ready_product(ReadyProducts *ready_products, ProductType type, int quantity);
int get_ready_product(ReadyProducts *ready_products, ProductType type, int quantity);
void lock_ready_products(void);
void unlock_ready_products(void);

#endif //INVENTORY_H