#ifndef BAKERTEAM_H
#define BAKERTEAM_H
#define NUM_BAKERY_TEAMS 3


typedef enum Team {
    BREAD_BAKERS = 0,
    CAKE_AND_SWEETS_BAKERS = 1,
    PASTRIES_BAKERS = 2
} Team;

typedef struct {
    Team team_name;
    int number_of_bakers;
} BakerTeam;



void init_team(BakerTeam *team, const char *name);

#endif // BAKERTEAM_H
