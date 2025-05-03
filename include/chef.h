//
// Created by yazan on 4/26/2025.
//

#ifndef CHEF_H
#define CHEF_H

#include <semaphore.h>
#include <inventory.h>
#include "products.h"

// Message structure for restock requests
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int urgency; // 0-10 scale, 10 being most urgent
} RestockRequest;


// Message structure for restock confirmations
typedef struct {
    long mtype;
    IngredientType ingredient;
    int quantity;
    int success; // 1 if successful, 0 if failed
} RestockConfirmation;

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


typedef enum {
    TEAM_PASTE,
    TEAM_BREAD,
    TEAM_CAKES,
    TEAM_SANDWICHES,
    TEAM_SWEETS,
    TEAM_SWEET_PATISSERIES,
    TEAM_SAVORY_PATISSERIES,
    TEAM_COUNT
} ChefTeam;

typedef struct {
    OrderItem item;
    ChefTeam source_team;
} PreparedItem;

typedef struct {
    long mtype;
    PreparedItem prepared_item;
} ChefMessage;

typedef struct {
    int id;
    ChefTeam team;
    pid_t pid;
    int is_active;
    int items_produced;
    ProductCategory* specialization;
    sem_t* inventory_sem;
    sem_t* ready_products_sem;
} Chef;

typedef struct {
    Chef chefs[MAX_CHEFS];
    int chef_count;
    int msg_queue_chefs;    // Queue for communication with chefs
    int msg_queue_bakers;   // Queue for communication with baker manager
    ProductCatalog* product_catalog;
    sem_t* inventory_sem;
    sem_t* ready_products_sem;
} ChefManager;


// Constants for inventory management
#define LOW_INGREDIENT_THRESHOLD 10
#define RESTOCK_TARGET_QUANTITY 50

// Function prototypes
void check_and_request_ingredients(ChefState *chef, Inventory *inventory);
void check_for_confirmations(ChefState *chef);
void prepare_recipes(ChefState *chef, Inventory *inventory, ReadyProducts *ready_products);
ChefManager* init_chef_manager(ProductCatalog* catalog, sem_t* inv_sem, sem_t* ready_sem);
void start_chef(Chef* chef, int msg_queue_id);
void process_chef_messages(ChefManager* manager, int* team_queues);
ChefTeam get_team_for_product_type(ProductType type);
ProductType get_product_type_for_team(ChefTeam team);
void simulate_chef_work(ChefTeam team, int msg_queue_id);
void calculate_production_ratios(const ReadyProducts *ready_products, float *ratios);
void reallocate_chefs(ChefManager* manager, int* team_queues, float* ratios);
void balance_teams(ChefManager *manager);
void handle_team_change(int signum);
#endif //CHEF_H
