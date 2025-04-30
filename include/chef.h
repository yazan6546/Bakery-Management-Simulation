//
// Created by yazan on 4/26/2025.
//

#ifndef CHEF_H
#define CHEF_H

#include <semaphore.h>

// Message structure for restock requests
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int urgency; // 0-10 scale, 10 being most urgent
} RestockRequest;


// Message structure for restock confirmations
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int success; // 1 if successful, 0 if failed
} RestockConfirmation;

// Chef state structure
typedef struct {
    int id;
    int request_queue_id;
    int response_queue_id;
    int is_waiting_for_ingredients;
    IngredientType waiting_for;
    int waiting_quantity;
    sem_t* inventory_sem;
    sem_t* ready_products_sem;
} ChefState;

// Constants for inventory management
#define LOW_INGREDIENT_THRESHOLD 10
#define RESTOCK_TARGET_QUANTITY 50

// Function prototypes
void check_and_request_ingredients(ChefState *chef, Inventory *inventory);
void check_for_confirmations(ChefState *chef);
void prepare_recipes(ChefState *chef, Inventory *inventory, ReadyProducts *ready_products);

#endif //CHEF_H
