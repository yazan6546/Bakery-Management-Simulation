#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "config.h"
#include "products.h"

// Function to create a test JSON file for products
void create_test_json_file(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Failed to create test JSON file\n");
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"products\": {\n");

    // Bread category
    fprintf(fp, "    \"bread\": [\n");
    fprintf(fp, "      {\n");
    fprintf(fp, "        \"id\": \"white_bread\",\n");
    fprintf(fp, "        \"name\": \"White Bread\",\n");
    fprintf(fp, "        \"price\": 2.5,\n");
    fprintf(fp, "        \"baking_time\": 20,\n");
    fprintf(fp, "        \"preparation_time\": 5,\n");
    fprintf(fp, "        \"ingredients\": [\n");
    fprintf(fp, "          {\"name\": \"wheat\", \"quantity\": 2},\n");
    fprintf(fp, "          {\"name\": \"yeast\", \"quantity\": 1},\n");
    fprintf(fp, "          {\"name\": \"salt\", \"quantity\": 0.5}\n");
    fprintf(fp, "        ]\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    ],\n");

    // Sandwich category
    fprintf(fp, "    \"sandwiches\": [\n");
    fprintf(fp, "      {\n");
    fprintf(fp, "        \"id\": \"cheese_sandwich\",\n");
    fprintf(fp, "        \"name\": \"Cheese Sandwich\",\n");
    fprintf(fp, "        \"price\": 5.0,\n");
    fprintf(fp, "        \"preparation_time\": 3,\n");
    fprintf(fp, "        \"ingredients\": [\n");
    fprintf(fp, "          {\"name\": \"bread\", \"quantity\": 2},\n");
    fprintf(fp, "          {\"name\": \"cheese\", \"quantity\": 1}\n");
    fprintf(fp, "        ]\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    ],\n");

    // Sweets category
    fprintf(fp, "    \"sweets\": [\n");
    fprintf(fp, "      {\n");
    fprintf(fp, "        \"id\": \"chocolate_cookie\",\n");
    fprintf(fp, "        \"name\": \"Chocolate Cookie\",\n");
    fprintf(fp, "        \"price\": 2.0,\n");
    fprintf(fp, "        \"preparation_time\": 4,\n");
    fprintf(fp, "        \"ingredients\": [\n");
    fprintf(fp, "          {\"name\": \"flour\", \"quantity\": 1},\n");
    fprintf(fp, "          {\"name\": \"sugar\", \"quantity\": 0.8},\n");
    fprintf(fp, "          {\"name\": \"chocolate\", \"quantity\": 0.5}\n");
    fprintf(fp, "        ]\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    ],\n");

    // Sweet patisseries
    fprintf(fp, "    \"sweet_patisseries\": [\n");
    fprintf(fp, "      {\n");
    fprintf(fp, "        \"id\": \"fruit_tart\",\n");
    fprintf(fp, "        \"name\": \"Fruit Tart\",\n");
    fprintf(fp, "        \"price\": 8.0,\n");
    fprintf(fp, "        \"preparation_time\": 6,\n");
    fprintf(fp, "        \"ingredients\": [\n");
    fprintf(fp, "          {\"name\": \"flour\", \"quantity\": 1},\n");
    fprintf(fp, "          {\"name\": \"sugar\", \"quantity\": 0.5},\n");
    fprintf(fp, "          {\"name\": \"butter\", \"quantity\": 0.5}\n");
    fprintf(fp, "        ]\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    ],\n");

    // Savory patisseries
    fprintf(fp, "    \"savory_patisseries\": [\n");
    fprintf(fp, "      {\n");
    fprintf(fp, "        \"id\": \"cheese_puff\",\n");
    fprintf(fp, "        \"name\": \"Cheese Puff\",\n");
    fprintf(fp, "        \"price\": 4.5,\n");
    fprintf(fp, "        \"preparation_time\": 5,\n");
    fprintf(fp, "        \"ingredients\": [\n");
    fprintf(fp, "          {\"name\": \"flour\", \"quantity\": 0.8},\n");
    fprintf(fp, "          {\"name\": \"cheese\", \"quantity\": 1.2}\n");
    fprintf(fp, "        ]\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    ]\n");

    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");

    fflush(fp);
    fclose(fp);
}

// Test string to enum conversion functions
void test_enum_conversions() {
    printf("Testing string to enum conversions:\n");

    printf("\nProduct Types:\n");
    printf("bread -> %d\n", get_product_type_from_string("bread"));
    printf("cake -> %d\n", get_product_type_from_string("cake"));
    printf("sandwiches -> %d\n", get_product_type_from_string("sandwiches"));
    printf("sweets -> %d\n", get_product_type_from_string("sweets"));
    printf("sweet_patisseries -> %d\n", get_product_type_from_string("sweet_patisseries"));
    printf("savory_patisseries -> %d\n", get_product_type_from_string("savory_patisseries"));
    printf("unknown_product -> %d (should be -1)\n", get_product_type_from_string("unknown_product"));

    printf("\nIngredient Types:\n");
    printf("wheat -> %d\n", get_ingredient_type_from_string("wheat"));
    printf("flour -> %d\n", get_ingredient_type_from_string("flour"));
    printf("yeast -> %d\n", get_ingredient_type_from_string("yeast"));
    printf("butter -> %d\n", get_ingredient_type_from_string("butter"));
    printf("cheese -> %d\n", get_ingredient_type_from_string("cheese"));
    printf("chocolate -> %d\n", get_ingredient_type_from_string("chocolate"));
    printf("unknown_ingredient -> %d (should be -1)\n", get_ingredient_type_from_string("unknown_ingredient"));
}

// Function to print out a product catalog
void print_product_catalog(ProductCatalog *catalog) {
    printf("\nProduct Catalog Contents:\n");
    printf("Total categories: %d\n", catalog->category_count);

    for (int i = 0; i < catalog->category_count; i++) {
        ProductCategory *category = &catalog->categories[i];
        printf("\nCategory %d: Type=%d, Products=%d\n", i, category->type, category->product_count);

        for (int j = 0; j < category->product_count; j++) {
            Product *product = &category->products[j];
            printf("  Product: %s (ID: %s), Price: %.2f, Prep Time: %d\n",
                   product->name, product->id, product->price, product->preparation_time);

            printf("    Ingredients (%d):\n", product->ingredient_count);
            for (int k = 0; k < product->ingredient_count; k++) {
                printf("      Type: %d, Quantity: %.1f\n",
                       product->ingredients[k].type, product->ingredients[k].quantity);
            }
        }
    }
}

int main() {

    const char *test_filename = "test_products.json";

    printf("=== JSON Loading Test ===\n");

    // Step 1: Create a test JSON file
    printf("Creating test JSON file: %s\n", test_filename);
    create_test_json_file(test_filename);

    // Step 2: Test enum conversion functions
    test_enum_conversions();

    // Step 3: Load and test product catalog
    printf("\nLoading product catalog from file...\n");
    ProductCatalog catalog;
    int result = load_product_catalog(test_filename, &catalog);

    if (result == 0) {
        printf("Successfully loaded product catalog!\n");
        print_product_catalog(&catalog);
    } else {
        printf("Failed to load product catalog. Error code: %d\n", result);
    }

    // Step 4: Test error handling
    printf("\nTesting error handling:\n");
    printf("Attempting to load from non-existent file: ");
    result = load_product_catalog("non_existent_file.json", &catalog);
    printf("%s\n", (result != 0) ? "Failed as expected" : "Unexpectedly succeeded");

    // Clean up
//    remove(test_filename);

    printf("\nJSON test completed.\n");
    return 0;
}