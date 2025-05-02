//
// Created by yazan on 5/1/2025.
//

#include "products.h"
#include "string.h"
#include <stdio.h>


// Utility function to convert string to ProductType enum
ProductType get_product_type_from_string(const char* name) {
    if (strcasecmp(name, "bread") == 0) return BREAD;
    if (strcasecmp(name, "cakes") == 0) return CAKE;
    if (strcasecmp(name, "sandwiches") == 0 || strcasecmp(name, "sandwich") == 0) return SANDWICH;
    if (strcasecmp(name, "sweets") == 0 || strcasecmp(name, "sweet") == 0) return SWEET;
    if (strcasecmp(name, "sweet_patisseries") == 0) return SWEET_PATISSERIES;
    if (strcasecmp(name, "savory_patisseries") == 0) return SAVORY_PATISSERIES;

    fprintf(stderr, "Unknown product type: %s\n", name);
    return -1; // Invalid type
}

// Utility function to convert string to IngredientType enum
IngredientType get_ingredient_type_from_string(const char* name) {
    if (strcasecmp(name, "wheat") == 0) return WHEAT;
    if (strcasecmp(name, "flour") == 0) return FLOUR;
    if (strcasecmp(name, "chocolate") == 0) return CHOCOLATE;
    if (strcasecmp(name, "yeast") == 0) return YEAST;
    if (strcasecmp(name, "butter") == 0) return BUTTER;
    if (strcasecmp(name, "milk") == 0) return MILK;
    if (strcasecmp(name, "sugar") == 0) return SUGAR;
    if (strcasecmp(name, "salt") == 0) return SALT;
    if (strcasecmp(name, "sweet_items") == 0) return SWEET_ITEMS;
    if (strcasecmp(name, "cheese") == 0) return CHEESE;
    if (strcasecmp(name, "salami") == 0) return SALAMI;
    if (strcasecmp(name, "paste_ingredients") == 0) return PASTE_INGREDIENTS;
    if (strcasecmp(name, "custard") == 0) return CUSTARD;
    if (strcasecmp(name, "vanilla") == 0) return VANILLA;
    if (strcasecmp(name, "eggs") == 0) return EGGS;
    if (strcasecmp(name, "vegetables") == 0) return VEGETABLES;
    if (strcasecmp(name, "bread") == 0) return BREAD_ING;
    if (strcasecmp(name, "cream") == 0) return CREAM;
    if (strcasecmp(name, "fresh_fruits") == 0) return FRUITS;

    fprintf(stderr, "Unknown ingredient type: %s\n", name);
    return -1; // Invalid type
}