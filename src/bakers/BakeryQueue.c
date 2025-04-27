#include "BakeryQueue.h"
#include <stdlib.h>
#include <string.h>

void init_bakery_queue(BakeryQueue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
}

void enqueue_bakery_item(BakeryQueue *queue, const char *item_name, const char *team_name) {
    BakeryItem *item = (BakeryItem *)malloc(sizeof(BakeryItem));
    strcpy(item->name, item_name);
    strcpy(item->team_name, team_name);
    item->next = NULL;

    if (queue->tail == NULL) {
        queue->head = queue->tail = item;
    } else {
        queue->tail->next = item;
        queue->tail = item;
    }
}

BakeryItem* dequeue_bakery_item(BakeryQueue *queue) {
    if (queue->head == NULL) return NULL;

    BakeryItem *item = queue->head;
    queue->head = item->next;
    if (queue->head == NULL)
        queue->tail = NULL;

    item->next = NULL;
    return item;
}

void return_item_front_bakery(BakeryQueue *queue, BakeryItem *item) {
    item->next = queue->head;
    queue->head = item;
    if (queue->tail == NULL)
        queue->tail = item;
}
