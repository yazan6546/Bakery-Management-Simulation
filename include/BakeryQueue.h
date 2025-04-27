#ifndef BAKERYQUEUE_H
#define BAKERYQUEUE_H

typedef struct BakeryItem {
    char name[50];
    char team_name[50];
    struct BakeryItem *next;
} BakeryItem;

typedef struct {
    BakeryItem *head;
    BakeryItem *tail;
} BakeryQueue;

void init_bakery_queue(BakeryQueue *queue);
void enqueue_bakery_item(BakeryQueue *queue, const char *item_name, const char *team_name);
BakeryItem* dequeue_bakery_item(BakeryQueue *queue);
void return_item_front_bakery(BakeryQueue *queue, BakeryItem *item);

#endif // BAKERYQUEUE_H
