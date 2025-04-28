#ifndef CHEF_H
#define CHEF_H
#include <stdbool.h>
#include <sys/types.h>

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

// Function prototypes
void initialize_chef_teams(ChefTeam *teams, int num_teams);
void create_chef_processes(ChefTeam *teams, int num_teams, BakeryIngredients *ingredients);
void cleanup_chef_processes(ChefTeam *teams, int num_teams);
const char* get_team_type_name(TeamType type);
void print_team_status(const ChefTeam *team);


#endif //CHEF_H
