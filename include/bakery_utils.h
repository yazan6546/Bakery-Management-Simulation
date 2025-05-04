// include/baker_utils.h
#ifndef BAKER_UTILS_H
#define BAKER_UTILS_H

#include "team.h"
#include "config.h"







// Get the team name string from the enum
const char* get_team_name_str(Team team);

// Serialize a baker team struct to a string
void serialize_baker_team(BakerTeam *team, char *buffer, size_t size);

// Deserialize a string into a baker team struct
void deserialize_baker_team(const char *buffer, BakerTeam *team);

pid_t start_process_baker(const char *binary, BakerTeam *team,
                         Config *config, int mqid_from_main, int mqid_ready);


void distribute_bakers_locally(Config *config, BakerTeam teams[NUM_BAKERY_TEAMS]);

#endif // BAKER_UTILS_H