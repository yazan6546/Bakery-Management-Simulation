#ifndef TEAM_H
#define TEAM_H

#include "products.h"
#include <semaphore.h>
#include <sys/types.h>


typedef enum Team {
    BREAD_BAKERS = 0,
    CAKE_AND_SWEETS_BAKERS = 1,
    PASTRIES_BAKERS = 2,
    NUM_BAKERY_TEAMS = 3
} Team;


typedef enum {
    TEAM_PASTE=6,
    TEAM_BREAD=0,
    TEAM_CAKES=1,
    TEAM_SANDWICHES=2,
    TEAM_SWEETS=3,
    TEAM_SWEET_PATISSERIES=4,
    TEAM_SAVORY_PATISSERIES=5,
    TEAM_COUNT=7
} ChefTeam;

typedef enum { BAKER_IDLE, BAKER_BUSY } State;

typedef struct {
    Team team_name;
    int number_of_bakers;
} BakerTeam;

typedef struct {
    int id;
    ChefTeam team;
    pid_t pid;
    int is_active;
    int items_produced;
    char Item[MAX_NAME_LENGTH];
    ProductCategory* specialization;
    sem_t* inventory_sem;
    sem_t* ready_products_sem;
} Chef;


typedef struct {
    Team team_name;         // Team name
    State state;           // State of the baker
    char Item[MAX_NAME_LENGTH]; // Item name
} Baker;


void init_team(BakerTeam *team, const char *name);
Team get_baker_team_from_chef_team(ChefTeam team);
ChefTeam get_team_for_product_type(ProductType type);


#endif // BAKERTEAM_H
