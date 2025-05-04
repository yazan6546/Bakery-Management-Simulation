//
// Created by yazan on 4/26/2025.
// Chef functions extracted for testing
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "inventory.h"
#include "config.h"
#include "game.h"
#include "chef.h"
#include "semaphores_utils.h"
#include "bakery_message.h"
#include "team.h"


// initialize manager
ChefManager* init_chef_manager(ProductCatalog* catalog, sem_t* inv_sem, sem_t* ready_sem) {
    ChefManager* manager = malloc(sizeof(ChefManager));
    if (!manager) {
        perror("Failed to allocate chef manager");
        return NULL;
    }

    manager->chef_count = 0;
    manager->product_catalog = catalog;
    manager->inventory_sem = inv_sem;
    manager->ready_products_sem = ready_sem;

    // Create message queue for baker communication
    manager->msg_queue_bakers = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (manager->msg_queue_bakers == -1) {
        perror("Failed to create baker message queue");
        free(manager);
        return NULL;
    }

    return manager;
}

// send messages between chef manager and chefs
void process_chef_messages(ChefManager* manager, int msg_queue, int baker_msg_queue, Game *game) {
    
      
    // prepare a message for receipt from the chefs  
    ChefMessage msg;
    while (msgrcv(msg_queue, &msg, sizeof(ChefMessage) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            // Forward to baker manager if needed
        if (msg.source_team != TEAM_SANDWICHES) {
        
            if (msgsnd(baker_msg_queue, &msg, sizeof(ChefMessage) - sizeof(long), 0) == -1) {
                perror("Failed to forward to baker manager");
            }
        } else {
            // Direct to ready products for items that don't need baking
            ProductType type = get_product_type_for_team(msg.source_team);
            add_ready_product(&game->ready_products,
                            type,
                            msg.product_index,
                            1,
                            manager->ready_products_sem);
        }
    }
    

        // Delay to prevent busy waiting
        usleep(100000);
}





// Function to simulate the work of a chef
void simulate_chef_work(ChefTeam team, int msg_queue_id, Game *game) {
    // Set up random seed based on process ID
    srand(time(NULL) ^ getpid());

    // Get inventory semaphores
    sem_t* inventory_sem = setup_inventory_semaphore();
    sem_t* ready_products_sem = setup_ready_products_semaphore();

    if (!inventory_sem || !ready_products_sem) {
        perror("Failed to setup semaphores");
        exit(1);
    }

    // Initialize chef state
    Chef chef = {
        .team = team,
        .is_active = 1, // Start as active
        .inventory_sem = inventory_sem,
        .ready_products_sem = ready_products_sem
    };

    printf("[Chef Worker] Started in team %d\n", team);

    while (1) {
        if (chef.team != team) {
            // Update team and specialization
            team = chef.team;
            printf("[Chef Worker] Switched to team %d\n", team);
        }
        // Get team's product category
        ProductCategory* category = &game->productCatalog.categories[team];

        if (category->product_count == 0) {
            sleep(1);
            continue;
        }

        // Select random product from category
        int product_index = rand() % category->product_count;
        Product* product = &category->products[product_index];

        // Check if we have enough ingredients
        lock_inventory(inventory_sem);
        int has_ingredients = 1;

        // Check if we have enough of each ingredient
        for (int i = 0; i < product->ingredient_count; i++) {
            if (game->inventory.quantities[product->ingredients[i].type] <
                product->ingredients[i].quantity) {
                has_ingredients = 0;
                break;
            }
        }

        // If we have enough ingredients, proceed with preparation
        // Otherwise, wait for ingredients
        if (has_ingredients) {
            // Use ingredients
            for (int i = 0; i < product->ingredient_count; i++) {
                game->inventory.quantities[product->ingredients[i].type] -=
                    product->ingredients[i].quantity;
            }
            unlock_inventory(inventory_sem);

            if (!chef.is_active) {
                chef.is_active = 1;
                printf("[Chef Worker Team %d] Waking up, ingredients available\n", team);
            }

            // Simulate preparation time
            printf("[Chef Worker Team %d] Starting production of %s\n",
                   team, product->name);
            sleep(product->preparation_time);

            // Handle products that don't need baking (sandwiches and paste)
            if (team == TEAM_SANDWICHES || team == TEAM_PASTE) {
                if (team == TEAM_SANDWICHES) {
                    // Handle sandwiches as before
                    ProductType product_type = get_product_type_for_team(team);
                    lock_ready_products(ready_products_sem);
                    add_ready_product(&game->ready_products,
                                    product_type,
                                    product_index,
                                    1,
                                    ready_products_sem);
                    unlock_ready_products(ready_products_sem);
                    printf("[Chef Worker Team %d] Added %s directly to ready products\n",
                           team, product->name);
                } else if (team == TEAM_PASTE) {
                    // Handle paste by adding to inventory
                    add_paste(&game->inventory, 1, inventory_sem);
                    printf("[Chef Worker Team %d] Added paste to inventory (total: %d)\n",
                           team, get_paste_count(&game->inventory, inventory_sem));
                }
            } else {
                // Prepare message for chef manager for items that need baking
                ChefMessage msg;
                msg.mtype = team + 1;  // Adding 1 to ensure mtype is positive
                msg.source_team = team;
                msg.product_index = product_index;
                strncpy(msg.product_name, product->name, MAX_NAME_LENGTH - 1);
                msg.product_name[MAX_NAME_LENGTH - 1] = '\0';

                // Send to chef manager
                if (msgsnd(msg_queue_id, &msg, sizeof(ChefMessage) - sizeof(long), 0) == -1) {
                    perror("[Chef Worker] Failed to send prepared item");
                } else {
                    printf("[Chef Worker Team %d] Sent %s to baker\n",
                           team, product->name);
                }
            }
        } else {
            unlock_inventory(inventory_sem);

            if (chef.is_active) {
                chef.is_active = 0;
                printf("[Chef Worker Team %d] Going to sleep, waiting for ingredients for %s\n",
                       team, product->name);
            }
            sleep(3); // Sleep while waiting for ingredients
        }
    }

    // Cleanup
    cleanup_ready_products_semaphore_resources(ready_products_sem);
    cleanup_inventory_semaphore_resources(inventory_sem);
}


// Function to calculate current production ratios
void calculate_production_ratios(const ReadyProducts *ready_products, float *ratios) {
    // Calculate total products for each category
    int totals[NUM_PRODUCTS] = {0};
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        for (int j = 0; j < ready_products->categories[i].product_count; j++) {
            totals[i] += ready_products->categories[i].quantities[j];
        }
    }

    // Calculate ratios relative to average
    float avg = 0;
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        avg += totals[i];
    }
    avg /= NUM_PRODUCTS;

    // Calculate ratio for each category
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        ratios[i] = (avg > 0) ? ((float)totals[i] / avg) : 1.0f;
    }
}


// Function to move a chef between teams
void move_chef(ChefManager *manager, ChefTeam from_team, ChefTeam to_team, Game *game) {
    // Find a chef from the source team
    for (int i = 0; i < manager->chef_count; i++) {
        Chef *chef = &manager->chefs[i];
        if (chef->team == from_team) {
            // Count chefs in source team to ensure minimum
            int source_team_count = 0;
            for (int j = 0; j < manager->chef_count; j++) {
                if (manager->chefs[j].team == from_team) {
                    source_team_count++;
                }
            }

            if (source_team_count > game->config.MIN_CHEFS_PER_TEAM) {
                // Move the chef to new team
                chef->team = to_team;
                printf("[Chef Manager] Moved chef %d from team %d to team %d\n",
                       chef->id, from_team, to_team);

                // Signal the chef to change team via kill
                kill(chef->pid, SIGUSR1);
                return;
            }
        }
    }
}

// Function to balance teams based on production
void balance_teams(ChefManager *manager, Game *game) {
    float production_ratios[NUM_PRODUCTS] = {0};
    calculate_production_ratios(&game->ready_products, production_ratios);

    // Find teams with highest and lowest production ratios
    float max_ratio = 0;
    float min_ratio = game->config.PRODUCTION_RATIO_THRESHOLD;
    ChefTeam max_team = 0;
    ChefTeam min_team = 0;

    for (int i = 0; i < NUM_PRODUCTS; i++) {
        if (production_ratios[i] > max_ratio) {
            max_ratio = production_ratios[i];
            max_team = get_team_for_product_type(i);
        }
        if (production_ratios[i] < min_ratio) {
            min_ratio = production_ratios[i];
            min_team = get_team_for_product_type(i);
        }
    }

    // If the ratio difference is significant, move a chef
    if (max_ratio / min_ratio > game->config.PRODUCTION_RATIO_THRESHOLD) {
        move_chef(manager, max_team, min_team, game);
    }
}

