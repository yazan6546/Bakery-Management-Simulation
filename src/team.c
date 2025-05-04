#include "team.h"
#include <stdio.h>

// Function to convert from chef team to baker team
Team get_baker_team_from_chef_team(ChefTeam team) {
    switch (team) {
        case TEAM_BREAD:
            return BREAD_BAKERS;
        case TEAM_CAKES:
        case TEAM_SWEETS:
            return CAKE_AND_SWEETS_BAKERS;
        case TEAM_SANDWICHES:
        case TEAM_SWEET_PATISSERIES:
        case TEAM_SAVORY_PATISSERIES:
            return PASTRIES_BAKERS;
        default:
            fprintf(stderr, "Unknown team type: %d\n", team);
            return -1;
    }
}