//
// Created by yazan on 4/27/2025.
//

#include "queue.h"
#include "BakeryItem.h"

int main() {
    BakeryItem item = {"Bread", "Team A"};
    queue *bakery_queue = createQueue(sizeof(BakeryItem));
    enqueue(bakery_queue, &item);
    enqueue(bakery_queue, &(BakeryItem){"wdew", "Team B"});

    BakeryItem temp;
    dequeue(bakery_queue, &temp);
    front(bakery_queue, &temp);
    printf("Front item: %s from %s\n", temp.name, temp.team_name);
}

void create_queue() {

}