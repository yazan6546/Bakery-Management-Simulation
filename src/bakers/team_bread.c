#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>

#include "BakerTeam.h"
#include "config.h"

#include "BakeryItem.h"
#include "oven.h"
#include "random.h"

typedef struct {
    long mtype;
    BakeryItem item;
} Message;

int main(int argc, char *argv[]) {


    char *buffer = argv[1];
    Config config;
    deserialize_config(&config, buffer);

    int mqid_from_main = atoi(argv[2]);
    int mqid_ready = atoi(argv[3]);


    init_random();

    int bakers_available = config.NUM_BAKERS / NUM_BAKERY_TEAMS;
    int busy_bakers = 0;

    while (1) {
        Message message;
        if (msgrcv(mqid_from_main, &message, sizeof(BakeryItem), 0, IPC_NOWAIT) >= 0) {
            BakeryItem *item = &message.item;

            if (strcmp(item->team_name, "Bake Bread") != 0) {
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

            int prep_time = (int)random_float(config.MIN_BAKE_TIME, config.MAX_BAKE_TIME);
            printf("[Bread Team] Preparing %s for %d seconds\n", item->name, prep_time);
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

