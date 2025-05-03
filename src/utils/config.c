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
    config->MAX_CUSTOMERS = -1;
    config->FRUSTRATED_CUSTOMERS = -1;
    config->COMPLAINED_CUSTOMERS = -1;
    config->CUSTOMERS_MISSING = -1;
    config->DAILY_PROFIT = -1;
    config->NUM_CHEFS = -1;
    config->NUM_BAKERS = -1;
    config->NUM_SELLERS = -1;
    config->NUM_SUPPLY_CHAIN = -1;
    config->MIN_PURCHASE_QUANTITY = -1;
    config->MAX_PURCHASE_QUANTITY = -1;
    config->MIN_TIME_FRUSTRATED = -1;
    config->MAX_TIME_FRUSTRATED = -1;
    config->MIN_OVEN_TIME = -1;
    config->MAX_OVEN_TIME = -1;
    config->NUM_OVENS = -1;
    config->MIN_BAKE_TIME= -1;
    config->MAX_BAKE_TIME= -1;
    config->MAX_PATIENCE = -1;
    config->MIN_PATIENCE = -1;
    config->MAX_PATIENCE_DECAY = -1;
    config->MIN_PATIENCE_DECAY = -1;
    config->CUSTOMER_PROBABILITY = -1;
    config->MIN_ORDER_ITEMS = -1;
    config->MAX_ORDER_ITEMS = -1;
    config->CUSTOMER_CASCADE_PROBABILITY = -1;
    config->CASCADE_WINDOW = -1;
    config->MIN_SELLER_PROCESSING_TIME;
    config->MAX_SELLER_PROCESSING_TIME;
    config->REALLOCATION_CHECK_INTERVAL = -1;
    config->PRODUCTION_RATIO_THRESHOLD = -1;
    config->MIN_CHEFS_PER_TEAM = -1;
    config->INGREDIENTS_TO_ORDER = -1;

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
            else if (strcmp(key, "MAX_CUSTOMERS") == 0) config->MAX_CUSTOMERS = (int) value;
            else if (strcmp(key, "MAX_TIME") == 0) config->MAX_TIME = (int)value;
            else if (strcmp(key, "COMPLAINED_CUSTOMERS") == 0) config->COMPLAINED_CUSTOMERS = (int)value;
            else if (strcmp(key, "CUSTOMERS_MISSING") == 0) config->CUSTOMERS_MISSING = (int)value;
            else if (strcmp(key, "DAILY_PROFIT") == 0) config->DAILY_PROFIT = value;
            else if (strcmp(key, "NUM_CHEFS") == 0) config->NUM_CHEFS = (int)value;
            else if (strcmp(key, "NUM_BAKERS") == 0) config->NUM_BAKERS = (int)value;
            else if (strcmp(key, "NUM_SELLERS") == 0) config->NUM_SELLERS = (int)value;
            else if (strcmp(key, "NUM_SUPPLY_CHAIN") == 0) config->NUM_SUPPLY_CHAIN = (int)value;
            else if (strcmp(key, "MIN_PURCHASE_QUANTITY") == 0) config->MIN_PURCHASE_QUANTITY = (int)value;
            else if (strcmp(key, "MAX_PURCHASE_QUANTITY") == 0) config->MAX_PURCHASE_QUANTITY = (int)value;
            else if (strcmp(key, "MIN_TIME_FRUSTRATED") == 0) config->MIN_TIME_FRUSTRATED = (int)value;
            else if (strcmp(key, "MAX_TIME_FRUSTRATED") == 0) config->MAX_TIME_FRUSTRATED = (int)value;
            else if (strcmp(key, "MIN_OVEN_TIME") == 0) config->MIN_OVEN_TIME = (int)value;
            else if (strcmp(key, "MAX_OVEN_TIME") == 0) config->MAX_OVEN_TIME = (int)value;
            else if (strcmp(key, "MAX_PATIENCE") == 0) config->MAX_PATIENCE = value;
            else if (strcmp(key, "MIN_PATIENCE") == 0) config->MIN_PATIENCE = value;
            else if (strcmp(key, "MAX_PATIENCE_DECAY") == 0) config->MAX_PATIENCE_DECAY = value;
            else if (strcmp(key, "MIN_PATIENCE_DECAY") == 0) config->MIN_PATIENCE_DECAY = value;
            else if (strcmp(key, "NUM_OVENS") == 0) config->NUM_OVENS = (int)value;
            else if (strcmp(key, "MIN_BAKE_TIME") == 0) config->MIN_BAKE_TIME = (int)value;
            else if (strcmp(key, "MAX_BAKE_TIME") == 0) config->MAX_BAKE_TIME = (int)value;
            else if (strcmp(key, "CUSTOMER_PROBABILITY") == 0) config->CUSTOMER_PROBABILITY = value;
            else if (strcmp(key, "MIN_ORDER_ITEMS") == 0) config->MIN_ORDER_ITEMS = (int)value;
            else if (strcmp(key, "MAX_ORDER_ITEMS") == 0) config->MAX_ORDER_ITEMS = (int)value;
            else if (strcmp(key, "CUSTOMER_CASCADE_PROBABILITY") == 0) config->CUSTOMER_CASCADE_PROBABILITY = value;
            else if (strcmp(key, "CASCADE_WINDOW") == 0) config->CASCADE_WINDOW = (int)value;
            else if (strcmp(key, "REALLOCATION_CHECK_INTERVAL") == 0) config->REALLOCATION_CHECK_INTERVAL = (int)value;
            else if (strcmp(key, "PRODUCTION_RATIO_THRESHOLD") == 0) config->PRODUCTION_RATIO_THRESHOLD = value;
            else if (strcmp(key, "MIN_CHEFS_PER_TEAM") == 0) config->MIN_CHEFS_PER_TEAM = (int)value;

            else if (strcmp(key, "MIN_SELLER_PROCESSING_TIME") == 0) config->MIN_SELLER_PROCESSING_TIME = (int)value;
            else if (strcmp(key, "MAX_SELLER_PROCESSING_TIME") == 0) config->MAX_SELLER_PROCESSING_TIME = (int)value;
            else if (strcmp(key, "INGREDIENTS_TO_ORDER") == 0) config->INGREDIENTS_TO_ORDER = (int)value;

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

void print_config(Config *config) {
    printf("Config values: \n");
    printf("MAX_TIME: %d\n", config->MAX_TIME);
    printf("MAX_CUSTOMERS: %d\n", config->MAX_CUSTOMERS);
    printf("FRUSTRATED_CUSTOMERS: %d\n", config->FRUSTRATED_CUSTOMERS);
    printf("COMPLAINED_CUSTOMERS: %d\n", config->COMPLAINED_CUSTOMERS);
    printf("CUSTOMERS_MISSING: %d\n", config->CUSTOMERS_MISSING);
    printf("DAILY_PROFIT: %f\n", config->DAILY_PROFIT);
    printf("NUM_CHEFS: %d\n", config->NUM_CHEFS);
    printf("NUM_BAKERS: %d\n", config->NUM_BAKERS);
    printf("NUM_SELLERS: %d\n", config->NUM_SELLERS);
    printf("NUM_SUPPLY_CHAIN: %d\n", config->NUM_SUPPLY_CHAIN);
    printf("MIN_PURCHASE_QUANTITY: %d\n", config->MIN_PURCHASE_QUANTITY);
    printf("MAX_PURCHASE_QUANTITY: %d\n", config->MAX_PURCHASE_QUANTITY);
    printf("MIN_TIME_FRUSTRATED: %d\n", config->MIN_TIME_FRUSTRATED);
    printf("MAX_TIME_FRUSTRATED: %d\n", config->MAX_TIME_FRUSTRATED);
    printf("MIN_OVEN_TIME: %d\n", config->MIN_OVEN_TIME);
    printf("MAX_OVEN_TIME: %d\n", config->MAX_OVEN_TIME);
    printf("NUM_OVENS: %d\n", config->NUM_OVENS);
    printf("MAX_PATIENCE: %f\n", config->MAX_PATIENCE);
    printf("MIN_PATIENCE: %f\n", config->MIN_PATIENCE);
    printf("MAX_PATIENCE_DECAY: %f\n", config->MAX_PATIENCE_DECAY);
    printf("MIN_PATIENCE_DECAY: %f\n", config->MIN_PATIENCE_DECAY);
    printf("MIN_BAKE_TIME: %d\n", config->MIN_BAKE_TIME);
    printf("MAX_BAKE_TIME: %d\n", config->MAX_BAKE_TIME);
    printf("CUSTOMER_PROBABILITY: %f\n", config->CUSTOMER_PROBABILITY);
    printf("MIN_ORDER_ITEMS: %d\n", config->MIN_ORDER_ITEMS);
    printf("MAX_ORDER_ITEMS: %d\n", config->MAX_ORDER_ITEMS);
    printf("INGREDIENTS_TO_ORDER: %d\n", config->INGREDIENTS_TO_ORDER);
    printf("CUSTOMER_CASCADE_PROBABILITY: %f\n", config->CUSTOMER_CASCADE_PROBABILITY);
    printf("CASCADE_WINDOW: %d\n", config->CASCADE_WINDOW);
    printf("MIN_SELLER_PROCESSING_TIME: %d\n", config->MIN_SELLER_PROCESSING_TIME);
    printf("MAX_SELLER_PROCESSING_TIME: %d\n", config->MAX_SELLER_PROCESSING_TIME);

    fflush(stdout);
}

int check_parameter_correctness(const Config *config) {

    // Check that all integer parameters are non-negative
    if (config->MAX_TIME < 0 || config->FRUSTRATED_CUSTOMERS < 0 || config->COMPLAINED_CUSTOMERS < 0 ||
        config->CUSTOMERS_MISSING < 0 || config->NUM_CHEFS < 0 || config->NUM_BAKERS < 0 || config->NUM_SELLERS < 0 ||
        config->NUM_SUPPLY_CHAIN < 0 || config->MIN_PURCHASE_QUANTITY < 0 || config->MAX_PURCHASE_QUANTITY < 0 ||
        config->MIN_TIME_FRUSTRATED < 0 || config->MAX_TIME_FRUSTRATED < 0 || config->MIN_OVEN_TIME < 0 ||
        config->MAX_OVEN_TIME < 0 || config->NUM_OVENS < 0 || config->MIN_BAKE_TIME < 0 || config->MAX_BAKE_TIME < 0 ||
        config->MAX_CUSTOMERS < 0 || config->MIN_ORDER_ITEMS < 0 || config->MAX_ORDER_ITEMS < 0 ||
        config->CASCADE_WINDOW < 0 || config->INGREDIENTS_TO_ORDER < 0
        config->CASCADE_WINDOW < 0 || config->MIN_SELLER_PROCESSING_TIME < 0
        || config->MAX_SELLER_PROCESSING_TIME < 0) {
        fprintf(stderr, "Values must be greater than or equal to 0\n");
        return -1;
    }

    // Check that float parameters are non-negative
    if (config->DAILY_PROFIT < 0 || config->MAX_PATIENCE < 0 || config->MIN_PATIENCE < 0 ||
        config->MAX_PATIENCE_DECAY < 0 || config->MIN_PATIENCE_DECAY < 0 || config->CUSTOMER_PROBABILITY < 0
        || config->CUSTOMER_CASCADE_PROBABILITY < 0) {
        fprintf(stderr, "Values must be greater than or equal to 0\n");
        return -1;
    }

    // Logical consistency checks for minimum and maximum pairs
    if (config->MIN_PURCHASE_QUANTITY > config->MAX_PURCHASE_QUANTITY) {
        fprintf(stderr, "MIN_PURCHASE_QUANTITY cannot be greater than MAX_PURCHASE_QUANTITY\n");
        return -1;
    }

    if (config->MIN_SELLER_PROCESSING_TIME > config->MAX_SELLER_PROCESSING_TIME) {
        fprintf(stderr, "MIN_SELLER_PROCESSING_TIME cannot be greater than MAX_SELLER_PROCESSING_TIME\n");
        return -1;
    }

    if (config->MIN_ORDER_ITEMS > config->MAX_ORDER_ITEMS) {
        fprintf(stderr, "MIN_ORDER_ITEMS cannot be greater than MAX_ORDER_ITEMS\n");
        return -1;
    }

    if (config->MAX_PATIENCE < config->MIN_PATIENCE) {
        fprintf(stderr, "MAX_PATIENCE cannot be less than MIN_PATIENCE\n");
        return -1;
    }

    if (config->MAX_PATIENCE_DECAY < config->MIN_PATIENCE_DECAY) {
        fprintf(stderr, "MAX_PATIENCE_DECAY cannot be less than MIN_PATIENCE_DECAY\n");
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
    sprintf(buffer, "%d %d %f %f %f %f %d %d %d %f %d %d %d %d %d %d %d %d %d %d %d %d %d %f %d %d %f %d %d %d",
            config->MAX_TIME,
            config->MAX_CUSTOMERS,
            config->MAX_PATIENCE,
            config->MIN_PATIENCE,
            config->MAX_PATIENCE_DECAY,
            config->MIN_PATIENCE_DECAY,
            config->FRUSTRATED_CUSTOMERS,
            config->COMPLAINED_CUSTOMERS,
            config->CUSTOMERS_MISSING,
            config->DAILY_PROFIT,
            config->NUM_CHEFS,
            config->NUM_BAKERS,
            config->NUM_SELLERS,
            config->NUM_SUPPLY_CHAIN,
            config->MIN_PURCHASE_QUANTITY,
            config->MAX_PURCHASE_QUANTITY,
            config->MIN_TIME_FRUSTRATED,
            config->MAX_TIME_FRUSTRATED,
            config->MIN_OVEN_TIME,
            config->MAX_OVEN_TIME,
            config->NUM_OVENS,
            config->MIN_BAKE_TIME,
            config->MAX_BAKE_TIME,
            config->CUSTOMER_PROBABILITY,
            config->MIN_ORDER_ITEMS,
            config->MAX_ORDER_ITEMS,
            config->CUSTOMER_CASCADE_PROBABILITY,
            config->CASCADE_WINDOW,
            config->MIN_SELLER_PROCESSING_TIME,
            config->MAX_SELLER_PROCESSING_TIME);
            config->INGREDIENTS_TO_ORDER);
}

void deserialize_config(const char *buffer, Config *config) {
    sscanf(buffer, "%d %d %f %f %f %f %d %d %d %f %d %d %d %d %d %d %d %d %d %d %d %d %d %f %d %d %f %d %d %d",
            &config->MAX_TIME,
            &config->MAX_CUSTOMERS,
            &config->MAX_PATIENCE,
            &config->MIN_PATIENCE,
            &config->MAX_PATIENCE_DECAY,
            &config->MIN_PATIENCE_DECAY,
            &config->FRUSTRATED_CUSTOMERS,
            &config->COMPLAINED_CUSTOMERS,
            &config->CUSTOMERS_MISSING,
            &config->DAILY_PROFIT,
            &config->NUM_CHEFS,
            &config->NUM_BAKERS,
            &config->NUM_SELLERS,
            &config->NUM_SUPPLY_CHAIN,
            &config->MIN_PURCHASE_QUANTITY,
            &config->MAX_PURCHASE_QUANTITY,
            &config->MIN_TIME_FRUSTRATED,
            &config->MAX_TIME_FRUSTRATED,
            &config->MIN_OVEN_TIME,
            &config->MAX_OVEN_TIME,
            &config->NUM_OVENS,
            &config->MIN_BAKE_TIME,
            &config->MAX_BAKE_TIME,
            &config->CUSTOMER_PROBABILITY,
            &config->MIN_ORDER_ITEMS,
            &config->MAX_ORDER_ITEMS,
            &config->CUSTOMER_CASCADE_PROBABILITY,
            &config->CASCADE_WINDOW,
            &config->MIN_SELLER_PROCESSING_TIME,
            &config->MAX_SELLER_PROCESSING_TIME);
            &config->INGREDIENTS_TO_ORDER);
}
