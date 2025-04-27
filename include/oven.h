#ifndef OVEN_H
#define OVEN_H

typedef struct {
    int id;
    int is_busy;
    int time_left;
    char item_name[50];
    char team_name[50];
} Oven;

void init_oven(Oven *oven, int id);
int put_item_in_oven(Oven *oven, const char *item_name, const char *team_name, int baking_time);
int oven_tick(Oven *oven);

#endif // OVEN_H
