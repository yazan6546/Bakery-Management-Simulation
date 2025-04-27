#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "oven.h"
#include "BakerTeam.h"
#include "BakeryQueue.h"

int main() {
    Config config;

    if (load_config("config.txt", &config) != 0) {
        printf("Failed to load config file.\n");
        return 1;
    }

    print_config(&config);

    srand(time(NULL)); // Random seed

    // Create ovens
    Oven ovens[config.NUM_OVENS];
    for (int i = 0; i < config.NUM_OVENS; i++) {
        init_oven(&ovens[i], i);
    }

    // Create baker teams
    BakerTeam teams[3];
    init_team(&teams[0], "Bake Bread");
    init_team(&teams[1], "Bake Cakes and Sweets");
    init_team(&teams[2], "Bake Sweet and Savory Patisseries");

    // Create bakery global queue
    BakeryQueue bakery_queue;
    init_bakery_queue(&bakery_queue);

    // Simulate 20 time units
    for (int t = 0; t < 20; t++) {
        printf("\n--- Time step %d ---\n", t + 1);

        // Teams randomly produce items
        for (int i = 0; i < 3; i++) {
            if (rand() % 2 == 0) {
                char item_name[50];
                sprintf(item_name, "Item-%d", rand() % 100);
                enqueue_bakery_item(&bakery_queue, item_name, teams[i].team_name);
                printf("Team %s produced %s\n", teams[i].team_name, item_name);
            }
        }

        // Try to put bakery items into ovens
        BakeryItem *item = dequeue_bakery_item(&bakery_queue);
        while (item != NULL) {
            int placed = 0;
            for (int j = 0; j < config.NUM_OVENS; j++) {
                if (!ovens[j].is_busy) {
                    int baking_time = config.MIN_OVEN_TIME + rand() % (config.MAX_OVEN_TIME - config.MIN_OVEN_TIME + 1);
                    put_item_in_oven(&ovens[j], item->name, item->team_name, baking_time);
                    printf("Placed %s in Oven %d for %d seconds.\n", item->name, ovens[j].id, baking_time);
                    placed = 1;
                    break;
                }
            }
            if (!placed) {
                printf("All ovens busy, %s is waiting.\n", item->name);
                return_item_front_bakery(&bakery_queue, item);
                break; // stop trying more items this second
            } else {
                free(item);
            }
            item = dequeue_bakery_item(&bakery_queue);
        }

        // Update ovens (tick)
        for (int j = 0; j < config.NUM_OVENS; j++) {
            if (oven_tick(&ovens[j])) {
                printf("Oven %d finished baking %s\n", ovens[j].id, ovens[j].item_name);
            }
        }

        sleep(1); // simulate 1 second
    }

    return 0;
}
