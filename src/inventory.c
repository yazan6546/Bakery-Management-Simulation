//
// Created by yazan on 4/26/2025.
//

#include <string.h>
#include <unistd.h>
#include "inventory.h"
#include "semaphores_utils.h"

// Initialize inventory
void init_inventory(Inventory *inventory) {
    // Initialize all quantities to zero
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
    inventory->max_capacity = 100; // Set a default max capacity
}

// Initialize ready products
void init_ready_products(ReadyProducts *ready_products) {
    // Initialize all categories and their quantities to zero
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        ready_products->categories[i].product_count = 0;
        for (int j = 0; j < MAX_PRODUCTS_PER_CATEGORY; j++) {
            ready_products->categories[i].quantities[j] = 0;
        }
    }
    ready_products->total_count = 0;
    ready_products->max_capacity = 50; // Set a default max capacity
}

// Add ingredient with thread safety
void add_ingredient(Inventory *inventory, IngredientType type, int quantity, sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }
    
    lock_inventory(sem);
    
    if ((type >= 0) && (type < NUM_INGREDIENTS)) {
        inventory->quantities[type] += quantity;
    }
    
    unlock_inventory(sem);
}

void add_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS], sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }
    
    lock_inventory(sem);
    
    // Add quantities from the array
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] += quantities[i];
    }
    
    unlock_inventory(sem);
}

int check_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS], sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }
    
    int result = 1;
    
    lock_inventory(sem);
    
    // Check if we have enough of each ingredient
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        if (inventory->quantities[i] < quantities[i]) {
            result = 0; // Not enough ingredients
            break;
        }
    }
    
    unlock_inventory(sem);
    
    return result;
}

void use_ingredients(Inventory *inventory, const int quantities[NUM_INGREDIENTS], sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }
    
    lock_inventory(sem);
    
    // Deduct used ingredients
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] -= quantities[i];
    }
    
    unlock_inventory(sem);
}

void restock_ingredients(Inventory *inventory, sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }
    
    lock_inventory(sem);
    
    // Reset all ingredients to 0
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        inventory->quantities[i] = 0;
    }
    
    unlock_inventory(sem);
}

void add_paste(Inventory *inventory, int quantity, sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }

    lock_inventory(sem);
    inventory->paste_count += quantity;
    unlock_inventory(sem);
}

int get_paste_count(Inventory *inventory, sem_t* sem) {
    if (!sem) {
        sem = setup_inventory_semaphore();
    }

    lock_inventory(sem);
    int count = inventory->paste_count;
    unlock_inventory(sem);

    return count;
}


// Add ready product with thread safety
void add_ready_product(ReadyProducts *ready_products, ProductType type, int product_index, int quantity, sem_t* sem) {
    if (!sem) {
        sem = setup_ready_products_semaphore();
    }

    lock_ready_products(sem);

    if (type >= 0 && type < NUM_PRODUCTS &&
        product_index >= 0 && product_index < MAX_PRODUCTS_PER_CATEGORY) {
        ready_products->categories[type].quantities[product_index] += quantity;
        ready_products->total_count += quantity;
    }

    unlock_ready_products(sem);
}

// Get ready product with thread safety
// Returns 1 if successful, 0 if not enough products
int get_ready_product(ReadyProducts *ready_products, ProductType type, int product_index, int quantity, sem_t* sem) {
    if (!sem) {
        sem = setup_ready_products_semaphore();
    }

    int result = 0;

    lock_ready_products(sem);

    if (type >= 0 && type < NUM_PRODUCTS &&
        product_index >= 0 && product_index < MAX_PRODUCTS_PER_CATEGORY &&
        ready_products->categories[type].quantities[product_index] >= quantity) {

        ready_products->categories[type].quantities[product_index] -= quantity;
        ready_products->total_count -= quantity;
        result = 1;
    }

    unlock_ready_products(sem);

    return result;
}



// Check if all products in an order are available and fulfill it if they are
// Returns 1 if order can be fulfilled, 0 otherwise
int check_and_fulfill_order(ReadyProducts *ready_products, CustomerOrder *order, sem_t* sem) {
    if (!sem) {
        sem = setup_ready_products_semaphore();
    }

    int can_fulfill = 1;

    // Lock the ready products for the entire operation
    lock_ready_products(sem);

    // First pass: check if all items are available without removing any
    for (int i = 0; i < order->item_count; i++) {
        OrderItem *item = &order->items[i];

        if (ready_products->categories[item->type].quantities[item->product_index] < item->quantity) {
            can_fulfill = 0;
            break;
        }
    }

    // Second pass: if all items are available, fulfill the order by reducing quantities
    if (can_fulfill) {
        for (int i = 0; i < order->item_count; i++) {
            OrderItem *item = &order->items[i];

            ready_products->categories[item->type].quantities[item->product_index] -= item->quantity;
            ready_products->total_count -= item->quantity;
        }
    }

    // Unlock the ready products
    unlock_ready_products(sem);

    return can_fulfill;
}

void print_inventory(Inventory *inventory) {
    printf("Inventory Contents:\n");
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        printf("  %s: %d units\n", get_ingredient_name(i), inventory->quantities[i]);
    }
    printf("-------------------------------\n");
}


// Utility function to convert ingredient type enum to string name
const char* get_ingredient_name(int ingredient_type) {
    switch(ingredient_type) {
        case WHEAT: return "Wheat";
        case FLOUR: return "Flour";
        case CHOCOLATE: return "Chocolate";
        case YEAST: return "Yeast";
        case BUTTER: return "Butter";
        case MILK: return "Milk";
        case SUGAR: return "Sugar";
        case SALT: return "Salt";
        case SWEET_ITEMS: return "Sweet Items";
        case CHEESE: return "Cheese";
        case SALAMI: return "Salami";
        case PASTE_INGREDIENTS: return "Paste Ingredients";
        case CUSTARD: return "Custard";
        case VANILLA: return "Vanilla";
        case EGGS: return "Eggs";
        case VEGETABLES: return "Vegetables";
        case BREAD_ING: return "Bread Ingredients";
        case CREAM: return "Cream";
        case FRUITS: return "Fresh Fruits";
        default: return "Unknown Ingredient";
    }
}