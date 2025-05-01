#include "config.h"
#include <json-c/json.h>
#include "products.h"

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

int load_product_catalog(const char *filename, ProductCatalog *catalog) {
    struct json_object *parsed_json;
    struct json_object *products_obj, *product_obj;
    struct json_object *ingredients_array, *ingredient_obj;
    struct json_object *temp;

    // Initialize catalog
    memset(catalog, 0, sizeof(ProductCatalog));

    // Parse JSON file
    parsed_json = json_object_from_file(filename);
    if (!parsed_json) {
        fprintf(stderr, "Failed to parse product JSON file: %s\n", filename);
        return -1;
    }

    // Get products object
    if (!json_object_object_get_ex(parsed_json, "products", &products_obj)) {
        fprintf(stderr, "No 'products' object found in JSON\n");
        json_object_put(parsed_json);
        return -1;
    }

    // Process each product category
    catalog->category_count = 0;
    json_object_object_foreach(products_obj, category_name, category_array) {
        if (catalog->category_count >= NUM_PRODUCTS) {
            fprintf(stderr, "Warning: Maximum number of categories reached\n");
            break;
        }

        ProductCategory *current_category = &catalog->categories[catalog->category_count];

        // Convert category name to enum type
        current_category->type = get_product_type_from_string(category_name);
        if (current_category->type < 0) {
            fprintf(stderr, "Skipping unknown category: %s\n", category_name);
            continue;
        }

        // Skip if not an array
        if (!json_object_is_type(category_array, json_type_array)) {
            continue;
        }

        // Process products in this category
        current_category->product_count = 0;
        int array_len = json_object_array_length(category_array);
        for (int i = 0; i < array_len; i++) {
            if (current_category->product_count >= MAX_PRODUCTS_PER_CATEGORY) {
                fprintf(stderr, "Warning: Maximum products per category reached for %s\n",
                        category_name);
                break;
            }

            product_obj = json_object_array_get_idx(category_array, i);
            Product *current_product = &current_category->products[current_category->product_count];

            // Get product ID
            if (json_object_object_get_ex(product_obj, "id", &temp)) {
                strncpy(current_product->id, json_object_get_string(temp), MAX_NAME_LENGTH - 1);
                current_product->id[MAX_NAME_LENGTH - 1] = '\0';
            }

            // Get product name
            if (json_object_object_get_ex(product_obj, "name", &temp)) {
                strncpy(current_product->name, json_object_get_string(temp), MAX_NAME_LENGTH - 1);
                current_product->name[MAX_NAME_LENGTH - 1] = '\0';
            }

            // Get product price
            if (json_object_object_get_ex(product_obj, "price", &temp)) {
                current_product->price = json_object_get_double(temp);
            }

            // Get preparation time
            if (json_object_object_get_ex(product_obj, "preparation_time", &temp)) {
                current_product->preparation_time = json_object_get_int(temp);
            }

            // Get ingredients
            current_product->ingredient_count = 0;
            if (json_object_object_get_ex(product_obj, "ingredients", &ingredients_array) &&
                json_object_is_type(ingredients_array, json_type_array)) {

                int ingredients_len = json_object_array_length(ingredients_array);
                for (int j = 0; j < ingredients_len && j < MAX_INGREDIENTS; j++) {
                    ingredient_obj = json_object_array_get_idx(ingredients_array, j);

                    // Get ingredient name and convert to enum type
                    if (json_object_object_get_ex(ingredient_obj, "name", &temp)) {
                        const char* ingredient_name = json_object_get_string(temp);
                        current_product->ingredients[j].type =
                            get_ingredient_type_from_string(ingredient_name);

                        if (current_product->ingredients[j].type < 0) {
                            fprintf(stderr, "Unknown ingredient type: %s\n", ingredient_name);
                            continue;
                        }
                    }

                    // Get ingredient quantity
                    if (json_object_object_get_ex(ingredient_obj, "quantity", &temp)) {
                        current_product->ingredients[j].quantity = json_object_get_double(temp);
                    }

                    current_product->ingredient_count++;
                }
            }

            current_category->product_count++;
        }

        catalog->category_count++;
    }

    // Free JSON object
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

// Utility function to convert string to ProductType enum
ProductType get_product_type_from_string(const char* name) {
    if (strcasecmp(name, "bread") == 0) return BREAD;
    if (strcasecmp(name, "cake") == 0) return CAKE;
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

    fprintf(stderr, "Unknown ingredient type: %s\n", name);
    return -1; // Invalid type
}
