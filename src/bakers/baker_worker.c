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

// 🔁 NEW: infer product type from both team and name
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
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mqid> <team_enum>\n", argv[0]);
        return 1;
    }

    int mqid = atoi(argv[1]);
    Team my_team = (Team)atoi(argv[2]);

    // Connect to shared memory
    int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
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
    init_random();

    while (1) {
        Message msg;
        if (msgrcv(mqid, &msg, sizeof(BakeryItem), 0, 0) >= 0) {
            if (!is_team_item(&msg.item, my_team)) {
                // Not my team’s responsibility
                msgsnd(mqid, &msg, sizeof(BakeryItem), 0);
                usleep(10000); // small wait to avoid busy loop
                continue;
            }

            int prep_time = (int)random_float(game->config.MIN_BAKE_TIME,game->config.MAX_BAKE_TIME);
            printf("[Baker %s] Preparing %s for %d seconds...\n",
                   get_team_name_str(my_team), msg.item.name, prep_time);
            sleep(prep_time);

            // 🔄 Try placing into oven
            int baked = 0;
            while (!baked) {
                for (int i = 0; i < game->config.NUM_OVENS; i++) {
                    if (!game->ovens[i].is_busy) {
                        int bake_time = game->config.MIN_OVEN_TIME +
                            rand() % (game->config.MAX_OVEN_TIME - game->config.MIN_OVEN_TIME + 1);
                        put_item_in_oven(&game->ovens[i], msg.item.name, msg.item.team_name, bake_time);

                        printf("[Baker %s] Placed %s in Oven %d for %d sec\n",
                               get_team_name_str(my_team), msg.item.name, i, bake_time);

                        sleep(bake_time); // simulate oven baking
                        printf("[Baker %s] Finished baking %s in Oven %d\n",
                               get_team_name_str(my_team), msg.item.name, i);

                        
                        ProductType ptype = infer_product_type(&msg.item);
                        add_ready_product(&game->readyProducts, ptype, 1);
                        baked = 1;
                        break;
                    }
                }
                if (!baked) sleep(1); // wait for an oven to become free
            }
        }
    }

    return 0;
}
