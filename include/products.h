//
// Created by yazan on 5/1/2025.
//

#ifndef PRODUCTS_H
#define PRODUCTS_H

#include "inventory.h"


#define MAX_NAME_LENGTH 64
#define MAX_INGREDIENTS 10
#define MAX_PRODUCTS_PER_CATEGORY 20
#define MAX_CATEGORIES 10

// Struct for an ingredient with quantity
typedef struct {
    IngredientType type; // Type of ingredient
    float quantity;
} Ingredient;

// Updated Product struct with separate timing fields
typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    float price;
    int preparation_time; // Time required for preparation (by chef)
    Ingredient ingredients[MAX_INGREDIENTS];
    int ingredient_count;
} Product;

// Category struct to hold products of the same type
typedef struct {
    ProductType type; // Type of product (e.g., bread, cake, etc.)
    Product products[MAX_PRODUCTS_PER_CATEGORY];
    int product_count;
} ProductCategory;

// ProductCatalog to hold all categories
typedef struct {
    ProductCategory categories[NUM_PRODUCTS];
    int category_count;
} ProductCatalog;


ProductType get_product_type_from_string(const char* name);
IngredientType get_ingredient_type_from_string(const char* name);
#endif //PRODUCTS_H
