#ifndef BAKERYQUEUE_H
#define BAKERYQUEUE_H

typedef struct BakeryItem {
    int name;
    char team_name[50];
} BakeryItem;


void backery_item_create(BakeryItem *item, char *name, char *team_name);

#endif // BAKERYQUEUE_H
