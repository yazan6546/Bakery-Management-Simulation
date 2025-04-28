//
// Created by yazan on 4/26/2025.
//

#ifndef INVENTORY_H
#define INVENTORY_H

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

// Inventory struct with array-based approach
typedef struct {
    int quantities[NUM_INGREDIENTS];  // Array of ingredient quantities
    int max_capacity;
} Inventory;

// Function prototypes
void init_inventory(Inventory *inventory);
void add_ingredient(Inventory *inventory, IngredientType type, int quantity);
void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS]);
void restock_ingredients(Inventory *inventory);

#endif //INVENTORY_H