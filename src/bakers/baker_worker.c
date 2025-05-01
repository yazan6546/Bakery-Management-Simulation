#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/mman.h>

#include "BakeryItem.h"
#include "oven.h"
#include "inventory.h"
#include "game.h"
#include "random.h"
#include "BakerTeam.h"
#include "bakery_utils.h"

typedef struct {
    long mtype;
    BakeryItem item;
} Message;

Game *game;

// Infer product type based on team and name
ProductType infer_product_type(BakeryItem *item) {
    if (strstr(item->team_name, "Bread")) {
        return BREAD;
    }
    if (strstr(item->team_name, "Cakes and Sweets")) {
        if (strstr(item->name, "Cake")) return CAKE;
        return SWEET;
    }
    if (strstr(item->team_name, "Patisseries")) {
        if (strstr(item->name, "Savory")) return SAVORY_PATISSERIES;
        return SWEET_PATISSERIES;
    }
    return BREAD; // fallback
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mqid> <team_enum>\n", argv[0]);
        return 1;
    }

    int mqid = atoi(argv[1]);
    Team my_team = (Team)atoi(argv[2]);

    int shm_fd = shm_open("/game_shm", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    close(shm_fd);

    setup_ready_products_semaphore();
    setup_oven_semaphores(game->config.NUM_OVENS);
    init_random();

    while (1) {
        // Tick ovens before checking messages
        for (int i = 0; i < game->config.NUM_OVENS; i++) {
            oven_tick(&game->ovens[i]);
        }

        Message msg;
        if (msgrcv(mqid, &msg, sizeof(BakeryItem), 0, IPC_NOWAIT) >= 0) {
            if (!is_team_item(&msg.item, my_team)) {
                msgsnd(mqid, &msg, sizeof(BakeryItem), 0);
                usleep(10000);
                continue;
            }

            int prep_time = (int)random_float(game->config.MIN_BAKE_TIME, game->config.MAX_BAKE_TIME);
            printf("[Baker %s] Preparing %s for %d seconds...\n",
                   get_team_name_str(my_team), msg.item.name, prep_time);
            sleep(prep_time);

            int baked = 0;
            while (!baked) {
                for (int i = 0; i < game->config.NUM_OVENS; i++) {
                    if (!game->ovens[i].is_busy) {
                        int bake_time = game->config.MIN_OVEN_TIME +
                                        rand() % (game->config.MAX_OVEN_TIME - game->config.MIN_OVEN_TIME + 1);
                        put_item_in_oven(&game->ovens[i], msg.item.name, msg.item.team_name, bake_time);

                        printf("[Baker %s] Placed %s in Oven %d for %d sec\n",
                               get_team_name_str(my_team), msg.item.name, i, bake_time);

                        ProductType ptype = infer_product_type(&msg.item);
                        add_ready_product(&game->ready_products, ptype, 1);

                        baked = 1;
                        break;
                    }
                }

                if (!baked) {
                    for (int i = 0; i < game->config.NUM_OVENS; i++) {
                        oven_tick(&game->ovens[i]);
                    }
                    sleep(1);
                }
            }
        } else {
            usleep(50000); // slight sleep to reduce CPU usage when queue is empty
        }
    }

    return 0;
}
