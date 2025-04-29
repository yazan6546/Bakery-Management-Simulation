#include "chef.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initialize_menu(Menu *menu, const Config *config) {
    // Calculate the total number of items
    menu->num_items = 6; // Example: Paste, Cakes, Sandwiches, Sweets, Sweet Patisserie, Savory Patisserie
    menu->items = malloc(menu->num_items * sizeof(MenuItem));

    // Initialize each menu item
    for (int i = 0; i < menu->num_items; i++) {
        menu->items[i].subtypes = NULL;

        if (i == 0) { // Cakes
            strcpy(menu->items[i].name, "Cakes");
            menu->items[i].num_subtypes = config->NUM_CAKE_FLAVORS;
        } else if (i == 1) { // Sweets
            strcpy(menu->items[i].name, "Sweets");
            menu->items[i].num_subtypes = config->NUM_SWEET_FLAVORS;
        } else if (i == 2) { // Sandwiches
            strcpy(menu->items[i].name, "Sandwiches");
            menu->items[i].num_subtypes = config->NUM_SANDWICH_CATEGORIES;
        } else if (i == 3) { // Sweet Patisserie
            strcpy(menu->items[i].name, "Sweet Patisserie");
            menu->items[i].num_subtypes = config->NUM_SWEET_PATISSERIES;
        } else if (i == 4) { // Savory Patisserie
            strcpy(menu->items[i].name, "Savory Patisserie");
            menu->items[i].num_subtypes = config->NUM_SAVORY_PATISSERIES;
        } else if (i == 5) { // Paste
            strcpy(menu->items[i].name, "Paste");
            menu->items[i].num_subtypes = config->NUM_PASTRY_CATEGORIES;
        }

        // Allocate memory for subtypes
        menu->items[i].subtypes = malloc(menu->items[i].num_subtypes * sizeof(char *));
        for (int j = 0; j < menu->items[i].num_subtypes; j++) {
            menu->items[i].subtypes[j] = malloc(50 * sizeof(char));
            snprintf(menu->items[i].subtypes[j], 50, "%s Type %d", menu->items[i].name, j + 1);
        }
    }
}

void print_menu(const Menu *menu) {
    printf("Bakery Menu:\n");
    for (int i = 0; i < menu->num_items; i++) {
        printf("%s:\n", menu->items[i].name);
        for (int j = 0; j < menu->items[i].num_subtypes; j++) {
            printf("  - %s\n", menu->items[i].subtypes[j]);
        }
    }
}

void free_menu(Menu *menu) {
    for (int i = 0; i < menu->num_items; i++) {
        for (int j = 0; j < menu->items[i].num_subtypes; j++) {
            free(menu->items[i].subtypes[j]);
        }
        free(menu->items[i].subtypes);
    }
    free(menu->items);
}