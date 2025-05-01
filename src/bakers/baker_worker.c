// ===== baker_worker.c =====

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>

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

typedef enum { IDLE, BUSY } BakerState;

ProductType infer_product_type(BakeryItem *item) {
    if (strstr(item->team_name, "Bread")) return BREAD;
    if (strstr(item->team_name, "Cakes and Sweets")) {
        if (strstr(item->name, "Cake")) return CAKE;
        return SWEET;
    }
    if (strstr(item->team_name, "Patisseries")) {
        if (strstr(item->name, "Savory")) return SAVORY_PATISSERIES;
        return SWEET_PATISSERIES;
    }
    return BREAD;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mqid> <team_enum>\n", argv[0]);
        return 1;
    }

    int mqid = atoi(argv[1]);
    Team my_team = (Team)atoi(argv[2]);

    int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open failed"); exit(1); }
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) { perror("mmap failed"); exit(1); }
    close(shm_fd);

    sem_t *ready_products_sem = setup_ready_products_semaphore();
    setup_oven_semaphores(game->config.NUM_OVENS);
    init_random();

    BakerState state = IDLE;
    BakeryItem current_item;
    int oven_index = -1;
    int oven_time_left = 0;

    while (1) {
        // Tick oven time
        if (state == BUSY && oven_index != -1) {
            oven_tick(&game->ovens[oven_index]);
            if (!game->ovens[oven_index].is_busy) {
                ProductType ptype = infer_product_type(&current_item);
                add_ready_product(&game->ready_products, ptype, 1, ready_products_sem);
                printf("[Baker %s] Finished baking %s in Oven %d\n", get_team_name_str(my_team), current_item.name, oven_index);
                state = IDLE;
                oven_index = -1;
            } else {
                sleep(1);
                continue;
            }
        }

        if (state == IDLE) {
            Message msg;
            if (msgrcv(mqid, &msg, sizeof(BakeryItem), 0, 0) >= 0) {
                if (!is_team_item(&msg.item, my_team)) {
                    msgsnd(mqid, &msg, sizeof(BakeryItem), 0);
                    usleep(10000);
                    continue;
                }

                current_item = msg.item;
                int prep_time = (int)random_float(game->config.MIN_BAKE_TIME, game->config.MAX_BAKE_TIME);
                printf("[Baker %s] Preparing %s for %d seconds...\n", get_team_name_str(my_team), current_item.name, prep_time);
                sleep(prep_time);

                int placed = 0;
                for (int i = 0; i < game->config.NUM_OVENS; i++) {
                    if (put_item_in_oven(&game->ovens[i], current_item.name, current_item.team_name,
                                         game->config.MIN_OVEN_TIME + rand() % (game->config.MAX_OVEN_TIME - game->config.MIN_OVEN_TIME + 1))) {
                        oven_index = i;
                        state = BUSY;
                        printf("[Baker %s] Placed %s in Oven %d\n", get_team_name_str(my_team), current_item.name, oven_index);
                        placed = 1;
                        break;
                    }
                }

                if (!placed) {
                    printf("[Baker %s] No oven available. Requeuing %s\n", get_team_name_str(my_team), current_item.name);
                    msgsnd(mqid, &msg, sizeof(BakeryItem), 0);
                    sleep(1);
                }
            } else {
                usleep(50000);
            }
        }
    }
    return 0;
}
