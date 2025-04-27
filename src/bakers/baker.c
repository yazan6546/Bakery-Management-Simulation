#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "queue.h"
#include "oven.h"
#include "BakerTeam.h"
#include "BakeryQueue.h"
#include "game.h"
#include <sys/mman.h>

Game *game;

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fd>\n", argv[0]);
        return 1;
    }

    int fd = atoi(argv[1]);  // 4th argument for mmap

    // Open the file descriptor for reading
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    fflush(stdout);
    Config config = game->config;

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

    queue *baker_queue = createQueue(sizeof(BakeryItem));

    // Simulate 20 time units
    for (int t = 0; t < 20; t++) {
        printf("\n--- Time step %d ---\n", t + 1);

        // Teams randomly produce items
        for (int i = 0; i < 3; i++) {
            if (rand() % 2 == 0) {
                char item_name[50];
                sprintf(item_name, "Item-%d", rand() % 100);
                BakeryItem item = {item_name, teams[i].team_name};
                enqueue(baker_queue, &item);
                printf("Team %s produced %s\n", teams[i].team_name, item_name);
            }
        }

        // Try to put bakery items into ovens
        BakeryItem item;
        queue *temp = front(baker_queue, &item);
        while (temp != NULL) {
            int placed = 0;
            for (int j = 0; j < config.NUM_OVENS; j++) {
                if (!ovens[j].is_busy) {
                    int baking_time = config.MIN_OVEN_TIME + rand() % (config.MAX_OVEN_TIME - config.MIN_OVEN_TIME + 1);
                    put_item_in_oven(&ovens[j], item.name, item.team_name, baking_time);
                    printf("Placed %s in Oven %d for %d seconds.\n", item.name, ovens[j].id, baking_time);
                    placed = 1;
                    break;
                }
            }
            if (!placed) {
                printf("All ovens busy, %s is waiting.\n", item.name);
                break; // stop trying more items this second
            }

            if (dequeue(baker_queue, &item) == NULL) {
                printf("deque failed");
                exit(EXIT_FAILURE);
            }
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
