#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>

#include "BakeryItem.h"
#include "oven.h"
#include "random.h"
#include "game.h"

typedef struct {
    long mtype;
    BakeryItem item;
} Message;

void handle_cakesweets_team(int mqid_from_main, int mqid_ready, Game *game) {
    init_random();

    int bakers_available = game->config.NUM_BAKERS / 3;
    int busy_bakers = 0;

    while (1) {
        Message message;
        if (msgrcv(mqid_from_main, &message, sizeof(BakeryItem), 0, IPC_NOWAIT) >= 0) {
            BakeryItem *item = &message.item;

            if (strcmp(item->team_name, "Bake Cakes and Sweets") != 0) {
                msgsnd(mqid_from_main, &message, sizeof(BakeryItem), 0);
                sleep(1);
                continue;
            }

            if (busy_bakers >= bakers_available) {
                msgsnd(mqid_from_main, &message, sizeof(BakeryItem), 0);
                sleep(1);
                continue;
            }

            busy_bakers++;

            int prep_time = (int)random_float(game->config.MIN_BAKE_TIME, game->config.MAX_BAKE_TIME);
            printf("[CakeSweets Team] Preparing %s for %d seconds\n", item->name, prep_time);
            sleep(prep_time);

            Message ready;
            ready.mtype = 1;
            ready.item = *item;
            msgsnd(mqid_ready, &ready, sizeof(BakeryItem), 0);

            busy_bakers--;
        } else {
            sleep(1);
        }
    }
}
