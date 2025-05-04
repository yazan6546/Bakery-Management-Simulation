//
// Created by yazan on 5/1/2025.
//

#ifndef PRODUCTS_H
#define PRODUCTS_H

#define MAX_ORDER_ITEMS_ 5
#define MAX_NAME_LENGTH 25
#define MAX_INGREDIENTS 10
#define MAX_PRODUCTS_PER_CATEGORY 20
#define MAX_CATEGORIES 10


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
    CHOCOLATE,
    FLOUR,
    VANILLA,
    CUSTARD,
    EGGS,
    VEGETABLES,
    BREAD_ING,
    CREAM,
    FRUITS,
    NUM_INGREDIENTS  // This will automatically equal the number of ingredients
} IngredientType;

typedef enum {
    BREAD=0,
    CAKE=1,
    SANDWICH=2,
    SWEET=3,
    SWEET_PATISSERIES=4,
    SAVORY_PATISSERIES=5,
    NUM_PRODUCTS=6  // This will automatically equal the number of products
} ProductType;


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



// An item in the order (product + quantity)
typedef struct {
    Product product;
    ProductType type;
    int product_index; // Index of the product in the category
    int quantity;
} OrderItem;

// The complete customer order
typedef struct {
    OrderItem items[MAX_ORDER_ITEMS_];
    int item_count;
    float total_price;
} CustomerOrder;

ProductType get_product_type_from_string(const char* name);
IngredientType get_ingredient_type_from_string(const char* name);
const char* get_ingredient_name(int ingredient_type);
#endif //PRODUCTS_H
