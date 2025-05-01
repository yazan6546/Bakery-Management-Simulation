#include "config.h"
#include <json-c/json.h>

// Function to load configuration settings from a specified file
int load_config(const char *filename, Config *config) {
    // Attempt to open the config file in read mode
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file"); // Print an error message if file opening fails
        return -1; // Return error code
    }

    // Initialize all configuration values to default or invalid values to indicate uninitialized state
    config->MAX_TIME = -1;
    config->FRUSTRATED_CUSTOMERS = -1;
    config->COMPLAINED_CUSTOMERS = -1;
    config->CUSTOMERS_MISSING = -1;
    config->DAILY_PROFIT = -1;
    config->NUM_BREAD_CATEGORIES = -1;
    config->NUM_SANDWICH_CATEGORIES = -1;
    config->NUM_CAKE_FLAVORS = -1;
    config->NUM_SWEET_CATEGORIES = -1;
    config->NUM_SWEET_FLAVORS = -1;
    config->NUM_SAVORY_PATISSERIES = -1;
    config->NUM_SWEET_PATISSERIES = -1;
    config->NUM_CHEFS = -1;
    config->NUM_BAKERS = -1;
    config->NUM_SELLERS = -1;
    config->NUM_SUPPLY_CHAIN = -1;
    config->NUM_PASTRY_CATEGORIES = -1;
    config->MIN_PURCHASE_QUANTITY = -1;
    config->MAX_PURCHASE_QUANTITY = -1;
    config->MIN_ITEM_PRICE = -1;
    config->MAX_ITEM_PRICE = -1;
    config->MIN_TIME_FRUSTRATED = -1;
    config->MAX_TIME_FRUSTRATED = -1;
    config->MIN_OVEN_TIME = -1;
    config->MAX_OVEN_TIME = -1;
    config->NUM_OVENS = -1;
    config->MIN_BAKE_TIME= -1;
    config->MAX_BAKE_TIME= -1;

    // Buffer to hold each line from the configuration file
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Ignore comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        // Parse each line as a key-value pair
        char key[50];
        float value;
        if (sscanf(line, "%40[^=]=%f", key, &value) == 2) {

            // Set corresponding config fields based on the key
            if (strcmp(key, "FRUSTRATED_CUSTOMERS") == 0) config->FRUSTRATED_CUSTOMERS = (int)value;
            else if (strcmp(key, "MAX_TIME") == 0) config->MAX_TIME = (int)value;
            else if (strcmp(key, "COMPLAINED_CUSTOMERS") == 0) config->COMPLAINED_CUSTOMERS = (int)value;
            else if (strcmp(key, "CUSTOMERS_MISSING") == 0) config->CUSTOMERS_MISSING = (int)value;
            else if (strcmp(key, "DAILY_PROFIT") == 0) config->DAILY_PROFIT = value;
            else if (strcmp(key, "NUM_BREAD_CATEGORIES") == 0) config->NUM_BREAD_CATEGORIES = (int)value;
            else if (strcmp(key, "NUM_SANDWICH_CATEGORIES") == 0) config->NUM_SANDWICH_CATEGORIES = (int)value;
            else if (strcmp(key, "NUM_CAKE_FLAVORS") == 0) config->NUM_CAKE_FLAVORS = (int)value;
            else if (strcmp(key, "NUM_SWEET_CATEGORIES") == 0) config->NUM_SWEET_CATEGORIES = (int)value;
            else if (strcmp(key, "NUM_SWEET_FLAVORS") == 0) config->NUM_SWEET_FLAVORS = (int)value;
            else if (strcmp(key, "NUM_SAVORY_PATISSERIES") == 0) config->NUM_SAVORY_PATISSERIES = (int)value;
            else if (strcmp(key, "NUM_SWEET_PATISSERIES") == 0) config->NUM_SWEET_PATISSERIES = (int)value;
            else if (strcmp(key, "NUM_CHEFS") == 0) config->NUM_CHEFS = (int)value;
            else if (strcmp(key, "NUM_BAKERS") == 0) config->NUM_BAKERS = (int)value;
            else if (strcmp(key, "NUM_SELLERS") == 0) config->NUM_SELLERS = (int)value;
            else if (strcmp(key, "NUM_SUPPLY_CHAIN") == 0) config->NUM_SUPPLY_CHAIN = (int)value;
            else if (strcmp(key, "NUM_PASTRY_CATEGORIES") == 0) config->NUM_PASTRY_CATEGORIES = (int)value;
            else if (strcmp(key, "MIN_PURCHASE_QUANTITY") == 0) config->MIN_PURCHASE_QUANTITY = (int)value;
            else if (strcmp(key, "MAX_PURCHASE_QUANTITY") == 0) config->MAX_PURCHASE_QUANTITY = (int)value;
            else if (strcmp(key, "MIN_ITEM_PRICE") == 0) config->MIN_ITEM_PRICE = value;
            else if (strcmp(key, "MAX_ITEM_PRICE") == 0) config->MAX_ITEM_PRICE = value;
            else if (strcmp(key, "MIN_TIME_FRUSTRATED") == 0) config->MIN_TIME_FRUSTRATED = (int)value;
            else if (strcmp(key, "MAX_TIME_FRUSTRATED") == 0) config->MAX_TIME_FRUSTRATED = (int)value;
            else if (strcmp(key, "MIN_OVEN_TIME") == 0) config->MIN_OVEN_TIME = (int)value;
            else if (strcmp(key, "MAX_OVEN_TIME") == 0) config->MAX_OVEN_TIME = (int)value;
            else if (strcmp(key, "NUM_OVENS") == 0) config->NUM_OVENS = (int)value;
            else if (strcmp(key, "MIN_BAKE_TIME") == 0) config->MIN_BAKE_TIME = (int)value;
            else if (strcmp(key, "MAX_BAKE_TIME") == 0) config->MAX_BAKE_TIME = (int)value;
            else {
                fprintf(stderr, "Unknown key: %s\n", key);
                fclose(file);
                return -1;
            }
        }
    }

    fclose(file); // Close the config file

    // Debug print if __CLI is defined
#ifdef __DEBUG
    printf("Config values: \n MIN_ENERGY: %d\n"
           "MAX_ENERGY: %d\n MAX_SCORE: %d\n"
           "MAX_TIME: %d\n NUM_ROUNDS: %d\n"
           "MIN_RATE_DECAY: %d\n MAX_RATE_DECAY: %d\n"
           , config->MIN_ENERGY,
           config->MAX_ENERGY,
           config->MAX_SCORE,
           config->MAX_TIME,
           config->NUM_ROUNDS,
           config->MIN_RATE_DECAY,
           config->MAX_RATE_DECAY);


fflush(stdout);
#endif


    // Check if all required parameters are set
    return check_parameter_correctness(config);

}


int load_config(const char *filename) {
    FILE *fp;
    char buffer[4096];
    struct json_object *parsed_json;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Failed to open config file");
        return -1;
    }

    fread(buffer, sizeof(char), sizeof(buffer), fp);
    fclose(fp);

    parsed_json = json_tokener_parse(buffer);
    if (parsed_json == NULL) {
        fprintf(stderr, "Failed to parse JSON config\n");
        return -1;
    }

    // Load simulation parameters
    struct json_object *simulation, *max_runtime, *max_frustrated, *max_complaints,
            *max_missing, *profit_target;

    json_object_object_get_ex(parsed_json, "simulation", &simulation);
    json_object_object_get_ex(simulation, "max_runtime_minutes", &max_runtime);
    json_object_object_get_ex(simulation, "max_frustrated_customers", &max_frustrated);
    json_object_object_get_ex(simulation, "max_complaints", &max_complaints);
    json_object_object_get_ex(simulation, "max_missing_item_requests", &max_missing);
    json_object_object_get_ex(simulation, "profit_target", &profit_target);

    bakery_data->max_runtime_minutes = json_object_get_int(max_runtime);
    bakery_data->max_frustrated_customers = json_object_get_int(max_frustrated);
    bakery_data->max_complaints = json_object_get_int(max_complaints);
    bakery_data->max_missing_item_requests = json_object_get_int(max_missing);
    bakery_data->profit_target = json_object_get_double(profit_target);

    // Load staff configuration
    struct json_object *staff, *chefs, *bakers, *sellers, *supply_chain;
    json_object_object_get_ex(parsed_json, "staff", &staff);
    json_object_object_get_ex(staff, "chefs", &chefs);
    json_object_object_get_ex(staff, "bakers", &bakers);
    json_object_object_get_ex(staff, "sellers", &sellers);
    json_object_object_get_ex(staff, "supply_chain", &supply_chain);

    // Load chef counts
    struct json_object *paste_team, *cake_team, *sandwich_team, *sweets_team,
            *sweet_patisserie_team, *savory_patisserie_team;
    json_object_object_get_ex(chefs, "paste_team", &paste_team);
    json_object_object_get_ex(chefs, "cake_team", &cake_team);
    json_object_object_get_ex(chefs, "sandwich_team", &sandwich_team);
    json_object_object_get_ex(chefs, "sweets_team", &sweets_team);
    json_object_object_get_ex(chefs, "sweet_patisserie_team", &sweet_patisserie_team);
    json_object_object_get_ex(chefs, "savory_patisserie_team", &savory_patisserie_team);

    bakery_data->worker_counts[CHEF_PASTE] = json_object_get_int(paste_team);
    bakery_data->worker_counts[CHEF_CAKE] = json_object_get_int(cake_team);
    bakery_data->worker_counts[CHEF_SANDWICH] = json_object_get_int(sandwich_team);
    bakery_data->worker_counts[CHEF_SWEET] = json_object_get_int(sweets_team);
    bakery_data->worker_counts[CHEF_SWEET_PATISSERIE] = json_object_get_int(sweet_patisserie_team);
    bakery_data->worker_counts[CHEF_SAVORY_PATISSERIE] = json_object_get_int(savory_patisserie_team);

    // Load baker counts
    struct json_object *bread_team, *cake_sweets_team, *patisserie_team;
    json_object_object_get_ex(bakers, "bread_team", &bread_team);
    json_object_object_get_ex(bakers, "cake_sweets_team", &cake_sweets_team);
    json_object_object_get_ex(bakers, "patisserie_team", &patisserie_team);

    bakery_data->worker_counts[BAKER_BREAD] = json_object_get_int(bread_team);
    bakery_data->worker_counts[BAKER_CAKE_SWEET] = json_object_get_int(cake_sweets_team);
    bakery_data->worker_counts[BAKER_PATISSERIE] = json_object_get_int(patisserie_team);

    // Load sellers and supply chain
    bakery_data->worker_counts[SELLER] = json_object_get_int(sellers);
    bakery_data->worker_counts[SUPPLY_CHAIN] = json_object_get_int(supply_chain);

    // Calculate total workers
    bakery_data->total_workers = 0;
    for (int i = 0; i < WORKER_TYPE_COUNT; i++) {
        bakery_data->total_workers += bakery_data->worker_counts[i];
    }

    // Load products
    struct json_object *products, *bread, *sandwiches, *cakes, *sweets,
            *sweet_patisseries, *savory_patisseries;

    json_object_object_get_ex(parsed_json, "products", &products);
    json_object_object_get_ex(products, "bread", &bread);
    json_object_object_get_ex(products, "sandwiches", &sandwiches);
    json_object_object_get_ex(products, "cakes", &cakes);
    json_object_object_get_ex(products, "sweets", &sweets);
    json_object_object_get_ex(products, "sweet_patisseries", &sweet_patisseries);
    json_object_object_get_ex(products, "savory_patisseries", &savory_patisseries);

    // Load bread products
    bakery_data->product_counts[BREAD] = json_object_array_length(bread);
    for (int i = 0; i < bakery_data->product_counts[BREAD]; i++) {
        struct json_object *item = json_object_array_get_idx(bread, i);
        struct json_object *name, *price, *production_time;

        json_object_object_get_ex(item, "name", &name);
        json_object_object_get_ex(item, "price", &price);
        json_object_object_get_ex(item, "production_time", &production_time);

        strncpy(bakery_data->products[BREAD][i].name, json_object_get_string(name), MAX_PRODUCT_NAME_LENGTH - 1);
        bakery_data->products[BREAD][i].price = json_object_get_double(price);
        bakery_data->products[BREAD][i].production_time = json_object_get_int(production_time);
        bakery_data->products[BREAD][i].available_quantity = 0;
        bakery_data->products[BREAD][i].in_progress = 0;
    }

    // Load sandwich products
    bakery_data->product_counts[SANDWICH] = json_object_array_length(sandwiches);
    for (int i = 0; i < bakery_data->product_counts[SANDWICH]; i++) {
        struct json_object *item = json_object_array_get_idx(sandwiches, i);
        struct json_object *name, *price, *production_time;

        json_object_object_get_ex(item, "name", &name);
        json_object_object_get_ex(item, "price", &price);
        json_object_object_get_ex(item, "production_time", &production_time);

        strncpy(bakery_data->products[SANDWICH][i].name, json_object_get_string(name), MAX_PRODUCT_NAME_LENGTH - 1);
        bakery_data->products[SANDWICH][i].price = json_object_get_double(price);
        bakery_data->products[SANDWICH][i].production_time = json_object_get_int(production_time);
        bakery_data->products[SANDWICH][i].available_quantity = 0;
        bakery_data->products[SANDWICH][i].in_progress = 0;
    }

    // Load cake products
    bakery_data->product_counts[CAKE] = json_object_array_length(cakes);
    for (int i = 0; i < bakery_data->product_counts[CAKE]; i++) {
        struct json_object *item = json_object_array_get_idx(cakes, i);
        struct json_object *name, *price, *production_time;

        json_object_object_get_ex(item, "name", &name);
        json_object_object_get_ex(item, "price", &price);
        json_object_object_get_ex(item, "production_time", &production_time);

        strncpy(bakery_data->products[CAKE][i].name, json_object_get_string(name), MAX_PRODUCT_NAME_LENGTH - 1);
        bakery_data->products[CAKE][i].price = json_object_get_double(price);
        bakery_data->products[CAKE][i].production_time = json_object_get_int(production_time);
        bakery_data->products[CAKE][i].available_quantity = 0;
        bakery_data->products[CAKE][i].in_progress = 0;
    }

    // Load sweet products
    bakery_data->product_counts[SWEET] = json_object_array_length(sweets);
    for (int i = 0; i < bakery_data->product_counts[SWEET]; i++) {
        struct json_object *item = json_object_array_get_idx(sweets, i);
        struct json_object *name, *price, *production_time;

        json_object_object_get_ex(item, "name", &name);
        json_object_object_get_ex(item, "price", &price);
        json_object_object_get_ex(item, "production_time", &production_time);

        strncpy(bakery_data->products[SWEET][i].name, json_object_get_string(name), MAX_PRODUCT_NAME_LENGTH - 1);
        bakery_data->products[SWEET][i].price = json_object_get_double(price);
        bakery_data->products[SWEET][i].production_time = json_object_get_int(production_time);
        bakery_data->products[SWEET][i].available_quantity = 0;
        bakery_data->products[SWEET][i].in_progress = 0;
    }

    // Load sweet patisserie products
    bakery_data->product_counts[SWEET_PATISSERIE] = json_object_array_length(sweet_patisseries);
    for (int i = 0; i < bakery_data->product_counts[SWEET_PATISSERIE]; i++) {
        struct json_object *item = json_object_array_get_idx(sweet_patisseries, i);
        struct json_object *name, *price, *production_time;

        json_object_object_get_ex(item, "name", &name);
        json_object_object_get_ex(item, "price", &price);
        json_object_object_get_ex(item, "production_time", &production_time);

        strncpy(bakery_data->products[SWEET_PATISSERIE][i].name, json_object_get_string(name), MAX_PRODUCT_NAME_LENGTH - 1);
        bakery_data->products[SWEET_PATISSERIE][i].price = json_object_get_double(price);
        bakery_data->products[SWEET_PATISSERIE][i].production_time = json_object_get_int(production_time);
        bakery_data->products[SWEET_PATISSERIE][i].available_quantity = 0;
        bakery_data->products[SWEET_PATISSERIE][i].in_progress = 0;
    }

    // Load savory patisserie products
    bakery_data->product_counts[SAVORY_PATISSERIE] = json_object_array_length(savory_patisseries);
    for (int i = 0; i < bakery_data->product_counts[SAVORY_PATISSERIE]; i++) {
        struct json_object *item = json_object_array_get_idx(savory_patisseries, i);
        struct json_object *name, *price, *production_time;

        json_object_object_get_ex(item, "name", &name);
        json_object_object_get_ex(item, "price", &price);
        json_object_object_get_ex(item, "production_time", &production_time);

        strncpy(bakery_data->products[SAVORY_PATISSERIE][i].name, json_object_get_string(name), MAX_PRODUCT_NAME_LENGTH - 1);
        bakery_data->products[SAVORY_PATISSERIE][i].price = json_object_get_double(price);
        bakery_data->products[SAVORY_PATISSERIE][i].production_time = json_object_get_int(production_time);
        bakery_data->products[SAVORY_PATISSERIE][i].available_quantity = 0;
        bakery_data->products[SAVORY_PATISSERIE][i].in_progress = 0;
    }

    // Load ingredients
    struct json_object *ingredients, *wheat, *yeast, *butter, *milk, *sugar, *salt,
            *sweet_items, *cheese, *salami;

    json_object_object_get_ex(parsed_json, "ingredients", &ingredients);
    json_object_object_get_ex(ingredients, "wheat", &wheat);
    json_object_object_get_ex(ingredients, "yeast", &yeast);
    json_object_object_get_ex(ingredients, "butter", &butter);
    json_object_object_get_ex(ingredients, "milk", &milk);
    json_object_object_get_ex(ingredients, "sugar", &sugar);
    json_object_object_get_ex(ingredients, "salt", &salt);
    json_object_object_get_ex(ingredients, "sweet_items", &sweet_items);
    json_object_object_get_ex(ingredients, "cheese", &cheese);
    json_object_object_get_ex(ingredients, "salami", &salami);

    // Load wheat
    struct json_object *wheat_stock, *wheat_min, *wheat_max;
    json_object_object_get_ex(wheat, "initial_stock", &wheat_stock);
    json_object_object_get_ex(wheat, "restock_min", &wheat_min);
    json_object_object_get_ex(wheat, "restock_max", &wheat_max);

    strcpy(bakery_data->ingredients[WHEAT].name, "Wheat");
    bakery_data->ingredients[WHEAT].quantity = json_object_get_int(wheat_stock);
    bakery_data->ingredients[WHEAT].restock_min = json_object_get_int(wheat_min);
    bakery_data->ingredients[WHEAT].restock_max = json_object_get_int(wheat_max);

    // Load yeast
    struct json_object *yeast_stock, *yeast_min, *yeast_max;
    json_object_object_get_ex(yeast, "initial_stock", &yeast_stock);
    json_object_object_get_ex(yeast, "restock_min", &yeast_min);
    json_object_object_get_ex(yeast, "restock_max", &yeast_max);

    strcpy(bakery_data->ingredients[YEAST].name, "Yeast");
    bakery_data->ingredients[YEAST].quantity = json_object_get_int(yeast_stock);
    bakery_data->ingredients[YEAST].restock_min = json_object_get_int(yeast_min);
    bakery_data->ingredients[YEAST].restock_max = json_object_get_int(yeast_max);

    // Load butter
    struct json_object *butter_stock, *butter_min, *butter_max;
    json_object_object_get_ex(butter, "initial_stock", &butter_stock);
    json_object_object_get_ex(butter, "restock_min", &butter_min);
    json_object_object_get_ex(butter, "restock_max", &butter_max);

    strcpy(bakery_data->ingredients[BUTTER].name, "Butter");
    bakery_data->ingredients[BUTTER].quantity = json_object_get_int(butter_stock);
    bakery_data->ingredients[BUTTER].restock_min = json_object_get_int(butter_min);
    bakery_data->ingredients[BUTTER].restock_max = json_object_get_int(butter_max);

    // Load milk
    struct json_object *milk_stock, *milk_min, *milk_max;
    json_object_object_get_ex(milk, "initial_stock", &milk_stock);
    json_object_object_get_ex(milk, "restock_min", &milk_min);
    json_object_object_get_ex(milk, "restock_max", &milk_max);

    strcpy(bakery_data->ingredients[MILK].name, "Milk");
    bakery_data->ingredients[MILK].quantity = json_object_get_int(milk_stock);
    bakery_data->ingredients[MILK].restock_min = json_object_get_int(milk_min);
    bakery_data->ingredients[MILK].restock_max = json_object_get_int(milk_max);

    // Load sugar
    struct json_object *sugar_stock, *sugar_min, *sugar_max;
    json_object_object_get_ex(sugar, "initial_stock", &sugar_stock);
    json_object_object_get_ex(sugar, "restock_min", &sugar_min);
    json_object_object_get_ex(sugar, "restock_max", &sugar_max);

    strcpy(bakery_data->ingredients[SUGAR].name, "Sugar");
    bakery_data->ingredients[SUGAR].quantity = json_object_get_int(sugar_stock);
    bakery_data->ingredients[SUGAR].restock_min = json_object_get_int(sugar_min);
    bakery_data->ingredients[SUGAR].restock_max = json_object_get_int(sugar_max);

    // Load salt
    struct json_object *salt_stock, *salt_min, *salt_max;
    json_object_object_get_ex(salt, "initial_stock", &salt_stock);
    json_object_object_get_ex(salt, "restock_min", &salt_min);
    json_object_object_get_ex(salt, "restock_max", &salt_max);

    strcpy(bakery_data->ingredients[SALT].name, "Salt");
    bakery_data->ingredients[SALT].quantity = json_object_get_int(salt_stock);
    bakery_data->ingredients[SALT].restock_min = json_object_get_int(salt_min);
    bakery_data->ingredients[SALT].restock_max = json_object_get_int(salt_max);

    // Load sweet items
    struct json_object *sweet_items_stock, *sweet_items_min, *sweet_items_max;
    json_object_object_get_ex(sweet_items, "initial_stock", &sweet_items_stock);
    json_object_object_get_ex(sweet_items, "restock_min", &sweet_items_min);
    json_object_object_get_ex(sweet_items, "restock_max", &sweet_items_max);

    strcpy(bakery_data->ingredients[SWEET_ITEMS].name, "Sweet Items");
    bakery_data->ingredients[SWEET_ITEMS].quantity = json_object_get_int(sweet_items_stock);
    bakery_data->ingredients[SWEET_ITEMS].restock_min = json_object_get_int(sweet_items_min);
    bakery_data->ingredients[SWEET_ITEMS].restock_max = json_object_get_int(sweet_items_max);

    // Load cheese
    struct json_object *cheese_stock, *cheese_min, *cheese_max;
    json_object_object_get_ex(cheese, "initial_stock", &cheese_stock);
    json_object_object_get_ex(cheese, "restock_min", &cheese_min);
    json_object_object_get_ex(cheese, "restock_max", &cheese_max);

    strcpy(bakery_data->ingredients[CHEESE].name, "Cheese");
    bakery_data->ingredients[CHEESE].quantity = json_object_get_int(cheese_stock);
    bakery_data->ingredients[CHEESE].restock_min = json_object_get_int(cheese_min);
    bakery_data->ingredients[CHEESE].restock_max = json_object_get_int(cheese_max);

    // Load salami
    struct json_object *salami_stock, *salami_min, *salami_max;
    json_object_object_get_ex(salami, "initial_stock", &salami_stock);
    json_object_object_get_ex(salami, "restock_min", &salami_min);
    json_object_object_get_ex(salami, "restock_max", &salami_max);

    strcpy(bakery_data->ingredients[SALAMI].name, "Salami");
    bakery_data->ingredients[SALAMI].quantity = json_object_get_int(salami_stock);
    bakery_data->ingredients[SALAMI].restock_min = json_object_get_int(salami_min);
    bakery_data->ingredients[SALAMI].restock_max = json_object_get_int(salami_max);

    // Free the JSON object
    json_object_put(parsed_json);

    return 0;
}


void print_config(Config *config) {
    printf("Config values: \n");
    printf("MAX_TIME: %d\n", config->MAX_TIME);
    printf("FRUSTRATED_CUSTOMERS: %d\n", config->FRUSTRATED_CUSTOMERS);
    printf("COMPLAINED_CUSTOMERS: %d\n", config->COMPLAINED_CUSTOMERS);
    printf("CUSTOMERS_MISSING: %d\n", config->CUSTOMERS_MISSING);
    printf("DAILY_PROFIT: %f\n", config->DAILY_PROFIT);
    printf("NUM_BREAD_CATEGORIES: %d\n", config->NUM_BREAD_CATEGORIES);
    printf("NUM_SANDWICH_CATEGORIES: %d\n", config->NUM_SANDWICH_CATEGORIES);
    printf("NUM_CAKE_FLAVORS: %d\n", config->NUM_CAKE_FLAVORS);
    printf("NUM_SWEET_CATEGORIES: %d\n", config->NUM_SWEET_CATEGORIES);
    printf("NUM_SWEET_FLAVORS: %d\n", config->NUM_SWEET_FLAVORS);
    printf("NUM_SAVORY_PATISSERIES: %d\n", config->NUM_SAVORY_PATISSERIES);
    printf("NUM_SWEET_PATISSERIES: %d\n", config->NUM_SWEET_PATISSERIES);
    printf("NUM_CHEFS: %d\n", config->NUM_CHEFS);
    printf("NUM_BAKERS: %d\n", config->NUM_BAKERS);
    printf("NUM_SELLERS: %d\n", config->NUM_SELLERS);
    printf("NUM_SUPPLY_CHAIN: %d\n", config->NUM_SUPPLY_CHAIN);
    printf("NUM_PASTRY_CATEGORIES: %d\n", config->NUM_PASTRY_CATEGORIES);
    printf("MIN_PURCHASE_QUANTITY: %d\n", config->MIN_PURCHASE_QUANTITY);
    printf("MAX_PURCHASE_QUANTITY: %d\n", config->MAX_PURCHASE_QUANTITY);
    printf("MIN_ITEM_PRICE: %f\n", config->MIN_ITEM_PRICE);
    printf("MAX_ITEM_PRICE: %f\n", config->MAX_ITEM_PRICE);
    printf("MIN_TIME_FRUSTRATED: %d\n", config->MIN_TIME_FRUSTRATED);
    printf("MAX_TIME_FRUSTRATED: %d\n", config->MAX_TIME_FRUSTRATED);
    printf("MIN_OVEN_TIME: %d\n", config->MIN_OVEN_TIME);
    printf("MAX_OVEN_TIME: %d\n", config->MAX_OVEN_TIME);
    printf("NUM_OVENS: %d\n", config->NUM_OVENS);
    printf("MIN_BAKE_TIME: %d\n", config->MIN_BAKE_TIME);
    printf("MAX_BAKE_TIME: %d\n", config->MAX_BAKE_TIME);

    fflush(stdout);
}

int check_parameter_correctness(const Config *config) {
    // Check that all integer parameters are non-negative
    if (config->MAX_TIME < 0 ||
        config->FRUSTRATED_CUSTOMERS < 0 ||
        config->COMPLAINED_CUSTOMERS < 0 ||
        config->CUSTOMERS_MISSING < 0 ||
        config->NUM_BREAD_CATEGORIES < 0 ||
        config->NUM_SANDWICH_CATEGORIES < 0 ||
        config->NUM_CAKE_FLAVORS < 0 ||
        config->NUM_SWEET_CATEGORIES < 0 ||
        config->NUM_SWEET_FLAVORS < 0 ||
        config->NUM_SAVORY_PATISSERIES < 0 ||
        config->NUM_SWEET_PATISSERIES < 0 ||
        config->NUM_CHEFS < 0 ||
        config->NUM_BAKERS < 0 ||
        config->NUM_SELLERS < 0 ||
        config->NUM_SUPPLY_CHAIN < 0 ||
        config->NUM_PASTRY_CATEGORIES < 0 ||
        config->MIN_PURCHASE_QUANTITY < 0 ||
        config->MAX_PURCHASE_QUANTITY < 0 ||
        config->MIN_TIME_FRUSTRATED < 0 ||
        config->MAX_TIME_FRUSTRATED < 0 ||
        config->MIN_OVEN_TIME < 0 ||
        config->MAX_OVEN_TIME < 0 ||
        config->NUM_OVENS < 0 ||
        config->MIN_BAKE_TIME < 0 ||
            config->MAX_BAKE_TIME < 0) {
        fprintf(stderr, "Values must be greater than or equal to 0\n");
        return -1;
    }

    // Check that float parameters are non-negative
    if (config->DAILY_PROFIT < 0 ||
        config->MIN_ITEM_PRICE < 0 ||
        config->MAX_ITEM_PRICE < 0) {
        fprintf(stderr, "Values must be greater than or equal to 0\n");
        return -1;
    }

    // Logical consistency checks for minimum and maximum pairs
    if (config->MIN_PURCHASE_QUANTITY > config->MAX_PURCHASE_QUANTITY) {
        fprintf(stderr, "MIN_PURCHASE_QUANTITY cannot be greater than MAX_PURCHASE_QUANTITY\n");
        return -1;
    }

    if (config->MIN_ITEM_PRICE > config->MAX_ITEM_PRICE) {
        fprintf(stderr, "MIN_ITEM_PRICE cannot be greater than MAX_ITEM_PRICE\n");
        return -1;
    }

    if (config->MIN_TIME_FRUSTRATED > config->MAX_TIME_FRUSTRATED) {
        fprintf(stderr, "MIN_TIME_FRUSTRATED cannot be greater than MAX_TIME_FRUSTRATED\n");
        return -1;
    }

    if (config->MIN_OVEN_TIME > config->MAX_OVEN_TIME) {
        fprintf(stderr, "MIN_OVEN_TIME cannot be greater than MAX_OVEN_TIME\n");
        return -1;
    }

    if (config->MIN_BAKE_TIME > config->MAX_BAKE_TIME) {
        fprintf(stderr, "MIN_BAKE_TIME cannot be greater than MAX_BAKE_TIME\n");
        return -1;
    }

    return 0;
}

void serialize_config(Config *config, char *buffer) {
    sprintf(buffer, "%d %d %d %d %f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %f %f %d %d %d %d %d %d %d",
            config->MAX_TIME,
            config->FRUSTRATED_CUSTOMERS,
            config->COMPLAINED_CUSTOMERS,
            config->CUSTOMERS_MISSING,
            config->DAILY_PROFIT,
            config->NUM_BREAD_CATEGORIES,
            config->NUM_SANDWICH_CATEGORIES,
            config->NUM_CAKE_FLAVORS,
            config->NUM_SWEET_CATEGORIES,
            config->NUM_SWEET_FLAVORS,
            config->NUM_SAVORY_PATISSERIES,
            config->NUM_SWEET_PATISSERIES,
            config->NUM_CHEFS,
            config->NUM_BAKERS,
            config->NUM_SELLERS,
            config->NUM_SUPPLY_CHAIN,
            config->NUM_PASTRY_CATEGORIES,
            config->MIN_PURCHASE_QUANTITY,
            config->MAX_PURCHASE_QUANTITY,
            config->MIN_ITEM_PRICE,
            config->MAX_ITEM_PRICE,
            config->MIN_TIME_FRUSTRATED,
            config->MAX_TIME_FRUSTRATED,
            config->MIN_OVEN_TIME,
            config->MAX_OVEN_TIME,
            config->NUM_OVENS,
            config->MIN_BAKE_TIME,
            config->MAX_BAKE_TIME);
}

void deserialize_config(const char *buffer, Config *config) {
    sscanf(buffer, "%d %d %d %d %f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %f %f %d %d %d %d %d %d %d",
            &config->MAX_TIME,
            &config->FRUSTRATED_CUSTOMERS,
            &config->COMPLAINED_CUSTOMERS,
            &config->CUSTOMERS_MISSING,
            &config->DAILY_PROFIT,
            &config->NUM_BREAD_CATEGORIES,
            &config->NUM_SANDWICH_CATEGORIES,
            &config->NUM_CAKE_FLAVORS,
            &config->NUM_SWEET_CATEGORIES,
            &config->NUM_SWEET_FLAVORS,
            &config->NUM_SAVORY_PATISSERIES,
            &config->NUM_SWEET_PATISSERIES,
            &config->NUM_CHEFS,
            &config->NUM_BAKERS,
            &config->NUM_SELLERS,
            &config->NUM_SUPPLY_CHAIN,
            &config->NUM_PASTRY_CATEGORIES,
            &config->MIN_PURCHASE_QUANTITY,
            &config->MAX_PURCHASE_QUANTITY,
            &config->MIN_ITEM_PRICE,
            &config->MAX_ITEM_PRICE,
            &config->MIN_TIME_FRUSTRATED,
            &config->MAX_TIME_FRUSTRATED,
            &config->MIN_OVEN_TIME,
            &config->MAX_OVEN_TIME,
            &config->NUM_OVENS,
            &config->MIN_BAKE_TIME,
            &config->MAX_BAKE_TIME);
}
