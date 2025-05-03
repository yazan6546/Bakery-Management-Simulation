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

Game *game;
int fd;
void start_chef(Chef* chef, int msg_queue_id) {
    printf("[Chef %d] Started working in team %d\n", chef->id, chef->team);

    while (1) {
        // Select a random product from chef's specialization
        if (chef->specialization->product_count == 0) {
            sleep(1);
            continue;
        }

        int product_index = rand() % chef->specialization->product_count;
        Product* product = &chef->specialization->products[product_index];

        // Check if we have enough ingredients
        if (check_ingredients(&game->inventory, product->ingredients, chef->inventory_sem)) {
            // Use ingredients
            use_ingredients(&game->inventory, product->ingredients, chef->inventory_sem);

            // Prepare the product
            sleep(product->preparation_time);

            // Create message for prepared item
            ChefMessage msg;
            msg.mtype = 1;
            msg.prepared_item.source_team = chef->team;
            msg.prepared_item.item.product = *product;
            msg.prepared_item.item.quantity = 1;

            // Send to chef manager
            if (msgsnd(msg_queue_id, &msg, sizeof(PreparedItem), 0) == -1) {
                perror("Failed to send item to chef manager");
            } else {
                chef->items_produced++;
                printf("[Chef %d] Prepared %s\n", chef->id, product->name);
            }
        }

        sleep(1); // Production interval
    }
}

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

void process_chef_messages(ChefManager* manager, int* team_queues) {
    while (1) {
        // Process messages from all team queues
        for (int team = 0; team < TEAM_COUNT; team++) {
            ChefMessage msg;
            while (msgrcv(team_queues[team], &msg, sizeof(PreparedItem), 0, IPC_NOWAIT) != -1) {
                // Forward to baker manager if needed
                if (msg.prepared_item.source_team != TEAM_SANDWICHES) {
                    if (msgsnd(manager->msg_queue_bakers, &msg, sizeof(PreparedItem), 0) == -1) {
                        perror("Failed to forward to baker manager");
                    }
                } else {
                    // Direct to ready products for items that don't need baking
                    add_ready_product(&game->ready_products,
                                   msg.prepared_item.item.product.id[0],
                                   0,
                                   msg.prepared_item.item.quantity,
                                   manager->ready_products_sem);
                }
            }
        }

        // Delay to prevent busy waiting
        usleep(100000);
    }
}

ProductType get_product_type_for_team(ChefTeam team) {
    switch (team) {
        case TEAM_BREAD:
            return BREAD;
        case TEAM_CAKES:
            return CAKE;
        case TEAM_SANDWICHES:
            return SANDWICH;
        case TEAM_SWEETS:
            return SWEET;
        case TEAM_SWEET_PATISSERIES:
            return SWEET_PATISSERIES;
        case TEAM_SAVORY_PATISSERIES:
            return SAVORY_PATISSERIES;
        case TEAM_PASTE:
            // Handle paste team as special case - no direct product type mapping
            return -1;
        default:
            fprintf(stderr, "Unknown team type: %d\n", team);
            return -1;
    }
}

ChefTeam get_team_for_product_type(ProductType type) {
    switch (type) {
        case BREAD:
            return TEAM_BREAD;
        case CAKE:
            return TEAM_CAKES;
        case SANDWICH:
            return TEAM_SANDWICHES;
        case SWEET:
            return TEAM_SWEETS;
        case SWEET_PATISSERIES:
            return TEAM_SWEET_PATISSERIES;
        case SAVORY_PATISSERIES:
            return TEAM_SAVORY_PATISSERIES;
        default:
            fprintf(stderr, "Unknown product type: %d\n", type);
            return -1;
    }
}

void simulate_chef_work(ChefTeam team, int msg_queue_id) {
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

        for (int i = 0; i < product->ingredient_count; i++) {
            if (game->inventory.quantities[product->ingredients[i].type] <
                product->ingredients[i].quantity) {
                has_ingredients = 0;
                break;
            }
        }

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
                msg.mtype = 1;
                msg.prepared_item.source_team = team;
                msg.prepared_item.item.product = *product;
                msg.prepared_item.item.quantity = 1;

                // Send to chef manager
                if (msgsnd(msg_queue_id, &msg, sizeof(PreparedItem), 0) == -1) {
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
    cleanup_semaphore_resources(inventory_sem, ready_products_sem);
}

//#define REALLOCATION_CHECK_INTERVAL 30  // Check every 30 seconds
//#define PRODUCTION_RATIO_THRESHOLD 1.5   // Trigger reallocation when ratio exceeds this
//#define MIN_CHEFS_PER_TEAM 1           // Minimum chefs that must remain in a team

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
void move_chef(ChefManager *manager, ChefTeam from_team, ChefTeam to_team) {
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
void balance_teams(ChefManager *manager) {
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
        move_chef(manager, max_team, min_team);
    }
}

void handle_team_change(int signum) {
    // Re-map shared memory to get updated team assignment
    // This is necessary because the team might have changed
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed during team change");
        exit(1);
    }
    printf("[Chef Worker] Received team reassignment signal\n");
}