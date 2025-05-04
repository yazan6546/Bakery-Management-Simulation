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

// Function to get the product type for a given team for adding to the products
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


// Function to get the team for a given product type
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
