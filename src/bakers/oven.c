#include "oven.h"
#include <string.h>
#include <stdio.h>


void init_oven(Oven *oven, int id) {
    oven->id = id;
    oven->is_busy = 0;
    oven->time_left = 0;
    oven->item_name[0] = '\0';
    oven->team_name[0] = '\0';
}

int put_item_in_oven(Oven *oven, const char *item_name, const char *team_name, int baking_time) {

    if (oven->is_busy)
        return 0;
    oven->is_busy = 1;
    oven->time_left = baking_time;
    strcpy(oven->item_name, item_name);
    strcpy(oven->team_name, team_name);
    return 1;
}

int oven_tick(Oven *oven) {



    if (oven->is_busy) {
        oven->time_left--;
        if (oven->time_left <= 0) {
            oven->is_busy = 0;
            printf("Oven %d finished baking %s\n", oven->id, oven->item_name);
            oven->item_name[0] = '\0';
            oven->team_name[0] = '\0'; 
            return 1;
        }
    }

    return 0;
}

