#ifndef BAKERY_MESSAGE_H
#define BAKERY_MESSAGE_H

#define MAX_ITEM_NAME 50
#define MAX_TEAM_NAME 50

typedef struct {
    long mtype; // Message type: 1 (Bread), 2 (Cake/Sweets), 3 (Patisseries)
    char item_name[MAX_ITEM_NAME];
    char team_name[MAX_TEAM_NAME];
    int bake_time;
} BakeryMessage;

#endif // BAKERY_MESSAGE_H
