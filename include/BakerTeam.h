#ifndef BAKERTEAM_H
#define BAKERTEAM_H

typedef struct {
    char team_name[50];
} BakerTeam;

void init_team(BakerTeam *team, const char *name);

#endif // BAKERTEAM_H
