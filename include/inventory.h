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

// Define names for shared memory file and semaphore
#define SHM_NAME "/bakery_inventory"
#define SEM_NAME "/bakery_inventory_sem"

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



// Shared memory globals
extern int shm_fd;
extern Inventory* shared_inventory;
extern sem_t* inventory_sem;

// Function prototypes for inventory operations
void init_inventory(Inventory *inventory);
void add_ingredient(Inventory *inventory, IngredientType type, int quantity);
void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
void restock_ingredients(Inventory *inventory);

// Function prototypes for shared memory and semaphores
int setup_shared_memory(void);
int setup_semaphore(void);
void lock_inventory(void);
void unlock_inventory(void);
void cleanup_resources(void);

#endif //INVENTORY_H