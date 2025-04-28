// src/bakers/baker_team.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include "config.h"
#include "BakerTeam.h"
#include "BakeryItem.h"
#include "random.h"
#include "bakery_utils.h"

typedef struct {
    long mtype;
    BakeryItem item;
} Message;

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <config_serialized> <mqid_team> <mqid_ready> <team_serialized>\n", argv[0]);
        return 1;
    }

    Config config;
    deserialize_config(argv[1], &config);

    int mqid_team = atoi(argv[2]);
    int mqid_ready = atoi(argv[3]);

    BakerTeam team;
    deserialize_baker_team(argv[4], &team);

    printf("Team %s started with %d bakers\n",
           get_team_name_str(team.team_name),
           team.number_of_bakers);

    init_random();

    BakeryItem current_item;
    int has_item = 0;
    int prep_time = 0;
    int remaining_time = 0;

    while (1) {
        if (!has_item) {
            Message incoming;
            if (msgrcv(mqid_team, &incoming, sizeof(BakeryItem), 0, IPC_NOWAIT) >= 0) {
                BakeryItem *item = &incoming.item;
                if (!is_team_item(item, team.team_name)) {
                    // Wrong team item
                    msgsnd(mqid_team, &incoming, sizeof(BakeryItem), 0);
                    usleep(5000);
                    continue;
                }

                current_item = *item;
                prep_time = (int)(random_float(config.MIN_BAKE_TIME, config.MAX_BAKE_TIME) / team.number_of_bakers);
                if (prep_time < 1) prep_time = 1;
                remaining_time = prep_time;

                printf("[%s] Preparing %s, estimated %d seconds\n",
                       get_team_name_str(team.team_name),
                       current_item.name,
                       prep_time);

                has_item = 1;
            }
        } else {
            if (remaining_time > 0) {
                remaining_time--;
                sleep(1);
            }
            if (remaining_time == 0) {
                Message ready;
                ready.mtype = 1;
                ready.item = current_item;
                if (msgsnd(mqid_ready, &ready, sizeof(BakeryItem), 0) == -1) {
                    perror("Failed to send ready item");
                } else {
                    printf("[%s] Finished preparing %s, sending to oven queue\n",
                           get_team_name_str(team.team_name),
                           current_item.name);
                }
                has_item = 0;
            }
        }

        usleep(10000);
    }

    return 0;
}
