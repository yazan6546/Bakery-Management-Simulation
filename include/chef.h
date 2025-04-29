#ifndef CHEF_H
#define CHEF_H
#include <stdbool.h>
#include <sys/types.h>
#include "config.h"
// Define team types
typedef enum {
    TEAM_PASTE,
    TEAM_CAKES,
    TEAM_SANDWICHES,
    TEAM_SWEETS,
    TEAM_SWEET_PATISSERIE,
    TEAM_SAVORY_PATISSERIE,
    NUM_TEAM_TYPES
} TeamType;

typedef enum {
    ITEM_PASTE,
    ITEM_CAKES,
    ITEM_SANDWICHES,
    ITEM_SWEETS,
    ITEM_SWEET_PATISSERIE,
    ITEM_SAVORY_PATISSERIE,
    NUM_MENU_ITEMS
} MenuItemName;

// Structure for bakery ingredients (will be in shared memory)
typedef struct {
    int wheat;
    int yeast;
    int butter;
    int milk;
    int sugar;
    int salt;
    int sweet_items;
    int cheese;
    int salami;
    int eggs;
} BakeryIngredients;

// Structure for chef team
typedef struct {
    pid_t pid;              // Process ID of this team
    TeamType type;          // Team type
    int team_id;            // Unique team identifier
    int team_size;          // Number of chefs in team
    bool is_active;         // Whether team is active
    int depends_on;         // Team ID this team depends on (-1 if none)
    int items_produced;     // Items produced count
    int items_required;     // Items needed from dependent team
} ChefTeam;

typedef struct {
    MenuItemName name;          // Enum
    char **subtypes;        // Dynamically allocated subtypes
    int num_subtypes;       // Number of subtypes
} MenuItem;


typedef struct {
    MenuItem *items;        // Dynamically allocated array of menu items
    int num_items;          // Number of items in the menu
} Menu;



// Function prototypes
void initialize_chef_teams(ChefTeam *teams, int num_teams);
void create_chef_processes(ChefTeam *teams, int num_teams, BakeryIngredients *ingredients);
void cleanup_chef_processes(ChefTeam *teams, int num_teams);
const char* get_team_type_name(TeamType type);
void print_team_status(const ChefTeam *team);
void initialize_menu(Menu *menu, const Config *config);
void print_menu(const Menu *menu);
void free_menu(Menu *menu);


#endif //CHEF_H
