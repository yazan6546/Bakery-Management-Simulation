//
// Created by yazan on 4/26/2025.
//

#ifndef CHEF_H
#define CHEF_H

#include <semaphore.h>
#include <inventory.h>
#include "products.h"
#include "game.h"
#include "team.h"




// Chef state structure
typedef struct {
    int id;
    int request_queue_id;
    int response_queue_id;
    int is_waiting_for_ingredients;
    IngredientType waiting_for;
    int waiting_quantity;
    sem_t* inventory_sem;
    sem_t* ready_products_sem;
} ChefState;


#define MAX_CHEFS 10
#define MAX_ITEMS_QUEUE 100







typedef struct {
    int chef_count;
    int msg_queue_chefs;    // Queue for communication with chefs
    int msg_queue_bakers;   // Queue for communication with baker manager
    ProductCatalog* product_catalog;
    sem_t* inventory_sem;
    sem_t* ready_products_sem;
} ChefManager;



// Function prototypes
void check_and_request_ingredients(ChefState *chef, Inventory *inventory);
void check_for_confirmations(ChefState *chef);
void prepare_recipes(ChefState *chef, Inventory *inventory, ReadyProducts *ready_products);
ChefManager* init_chef_manager(ProductCatalog* catalog, sem_t* inv_sem, sem_t* ready_sem);
void start_chef(Chef* chef, int msg_queue_id);
void process_chef_messages(ChefManager* manager, int msg_queue, int baker_msg_queue, struct Game *game);
ChefTeam get_team_for_product_type(ProductType type);
ProductType get_product_type_for_team(ChefTeam team);
void simulate_chef_work(ChefTeam team, int msg_queue_id, struct Game *game, int id);
void calculate_production_ratios(const ReadyProducts *ready_products, float *ratios);
void reallocate_chefs(ChefManager* manager, int msg_queue, float* ratios);
void balance_teams(struct Game *game);
void handle_team_change(int signum);
void move_chef(ChefTeam from_team, ChefTeam to_team, struct Game *game);


#endif //CHEF_H
