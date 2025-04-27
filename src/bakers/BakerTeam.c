#include "BakerTeam.h"
#include <string.h>

void init_team(BakerTeam *team, const char *name) {
    strcpy(team->team_name, name);
}
