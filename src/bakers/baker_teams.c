// src/bakers/baker_team.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>

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
        printf("Usage: %s <config> <mqid_from_main> <mqid_ready> <team_data>\n", argv[0]);
        return 1;
    }

    // Parse arguments
    Config config;
    deserialize_config(&config, argv[1]);

    int mqid_from_main = atoi(argv[2]);
    int mqid_ready = atoi(argv[3]);

    // Parse team data
    BakerTeam team;
    deserialize_baker_team(argv[4], &team);

    // Print team info
    printf("Baker team initialized: %s with %d bakers\n",
           get_team_name_str(team.team_name),
           team.number_of_bakers);

    init_random();

    // Track busy bakers
    int busy_bakers = 0;

    while (1) {
        Message message;
        if (msgrcv(mqid_from_main, &message, sizeof(BakeryItem), 0, IPC_NOWAIT) >= 0) {
            BakeryItem *item = &message.item;

            // Check if this item belongs to this team
            if (!is_team_item(item, team.team_name)) {
                // Not our item, requeue it
                msgsnd(mqid_from_main, &message, sizeof(BakeryItem), 0);
                usleep(10000); // Small delay to prevent CPU spinning
                continue;
            }

            // Check if team has available bakers
            if (busy_bakers >= team.number_of_bakers) {
                // No available bakers, requeue the message
                msgsnd(mqid_from_main, &message, sizeof(BakeryItem), 0);
                usleep(100000); // Brief delay
                continue;
            }

            const char* team_name = get_team_name_str(team.team_name);

            int prep_time = (int)random_float(config.MIN_BAKE_TIME, config.MAX_BAKE_TIME);
            printf("[%s] Preparing %s for %d seconds\n", team_name, item->name, prep_time);
            sleep(prep_time);

            // Send the item to be baked
            Message ready;
            ready.mtype = 1;
            ready.item = *item;
            msgsnd(mqid_ready, &ready, sizeof(BakeryItem), 0);

        } else {
            // No messages available, brief pause before checking again
            usleep(100000);
        }
    }
}