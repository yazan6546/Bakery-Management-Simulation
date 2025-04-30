#include "bakery_utils.h"
#include "BakerTeam.h"
#include "random.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>  // For fork() and execl()
#include <sys/types.h> // For pid_t


const char* get_team_name_str(Team team) {
    switch (team) {
        case BREAD_BAKERS:
            return "Bake Bread";
        case CAKE_AND_SWEETS_BAKERS:
            return "Bake Cakes and Sweets";
        case PASTRIES_BAKERS:
            return "Bake Sweet and Savory Patisseries";
        default:
            return "Unknown Team";
    }
}

int is_team_item(BakeryItem* item, Team team) {
    const char* team_name_str = get_team_name_str(team);
    return strcmp(item->team_name, team_name_str) == 0;
}

void serialize_baker_team(BakerTeam *team, char *buffer, size_t size) {
    snprintf(buffer, size, "%d,%d", (int)team->team_name, team->number_of_bakers);
}

void deserialize_baker_team(const char *buffer, BakerTeam *team) {
    int team_value;
    sscanf(buffer, "%d,%d", &team_value, &team->number_of_bakers);
    team->team_name = (Team)team_value;
}

// Distributes bakers among the three teams
void distribute_bakers_locally(Config* config, BakerTeam teams[NUM_BAKERY_TEAMS]) {
    int remaining_bakers = config->NUM_BAKERS;
    int min_bakers_per_team = 1; // Ensure at least one baker per team

    // Initialize teams with minimum bakers
    for (int i = 0; i < NUM_BAKERY_TEAMS; i++) {
        teams[i].team_name = i; // Enum value corresponds to index
        teams[i].number_of_bakers = min_bakers_per_team;
        remaining_bakers -= min_bakers_per_team;
    }

    // Distribute remaining bakers randomly
    while (remaining_bakers > 0) {
        int team_index = rand() % NUM_BAKERY_TEAMS;
        teams[team_index].number_of_bakers++;
        remaining_bakers--;
    }

    // Print distribution
    printf("Baker distribution: ");
    for (int i = 0; i < NUM_BAKERY_TEAMS; i++) {
        printf("%s: %d", get_team_name_str(teams[i].team_name), teams[i].number_of_bakers);
        if (i < NUM_BAKERY_TEAMS - 1) printf(", ");
    }
    printf("\n");
}

pid_t start_process_baker(const char *binary, BakerTeam *team,
                         Config *config, int mqid_from_main, int mqid_ready) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Serialize config
        char config_buffer[512];
        serialize_config(config, config_buffer);

        // Convert message queue IDs to strings
        char mqid_from_main_str[16];
        char mqid_ready_str[16];
        snprintf(mqid_from_main_str, sizeof(mqid_from_main_str), "%d", mqid_from_main);
        snprintf(mqid_ready_str, sizeof(mqid_ready_str), "%d", mqid_ready);

        // Serialize team data
        char team_buffer[32];
        serialize_baker_team(team, team_buffer, sizeof(team_buffer));

        if (execl(binary, binary, config_buffer, mqid_from_main_str, mqid_ready_str, team_buffer, NULL)) {
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}
