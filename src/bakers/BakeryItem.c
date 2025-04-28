//
// Created by yazan on 4/27/2025.
//

#include "BakeryItem.h"

#include <string.h>

void backery_item_create(BakeryItem *item, char *name, char *team_name) {
    strcpy(item->name, name);
    strcpy(item->team_name, team_name);
}
