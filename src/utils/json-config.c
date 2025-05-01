//
// Created by yazan on 5/1/2025.
//

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include "products.h"


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