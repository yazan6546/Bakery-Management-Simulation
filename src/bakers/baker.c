#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <time.h>

#include "queue.h"
#include "oven.h"
#include "BakeryItem.h"
#include "BakerTeam.h"
#include "game.h"
#include "random.h"
#include "bakeryteam.h"

typedef struct {
    long mtype;
    BakeryItem item;
} Message;

Game *game;
pid_t start_process_baker(const char *binary, Config *config, int mqid_from_main, int mqid_ready);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fd>\n", argv[0]);
        return 1;
    }

    int fd = atoi(argv[1]);
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    printf("********** Bakery Simulation **********\n");
    Config config = game->config;
    print_config(&config);
    init_random();

    // 👇 FIX: Initialize ovens cleanly
    for (int i = 0; i < config.NUM_OVENS; i++) {
        init_oven(&game->ovens[i], i);
    }

    int mqid_from_main = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    int mqid_ready = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    if (mqid_from_main == -1 || mqid_ready == -1) {
        perror("msgget failed");
        exit(1);
    }



    // pid_t bread_pid = fork();
    // if (bread_pid == 0) {
    //     handle_bread_team(mqid_from_main, mqid_ready, game);
    //     exit(0);
    // }
    //
    // pid_t cakesweets_pid = fork();
    // if (cakesweets_pid == 0) {
    //     handle_cakesweets_team(mqid_from_main, mqid_ready, game);
    //     exit(0);
    // }
    //
    // pid_t patisseries_pid = fork();
    // if (patisseries_pid == 0) {
    //     handle_patisseries_team(mqid_from_main, mqid_ready, game);
    //     exit(0);
    // }

    start_process_baker("./baker_team", &game->config, mqid_from_main, mqid_ready);

    for (int t = 0;; t++) {
        printf("\n=== Main Time Step %d ===\n", t + 1);

        if (rand() % 2 == 0) {
            char item_name[50];
            sprintf(item_name, "Item-%d", rand() % 100);

            BakeryItem item;
            int team = rand() % 3;
            if (team == 0)
                backery_item_create(&item, item_name, "Bake Bread");
            else if (team == 1)
                backery_item_create(&item, item_name, "Bake Cakes and Sweets");
            else
                backery_item_create(&item, item_name, "Bake Sweet and Savory Patisseries");

            Message msg;
            msg.mtype = 1;
            msg.item = item;
            if (msgsnd(mqid_from_main, &msg, sizeof(BakeryItem), 0) == -1) {
                perror("msgsnd main->team failed");
            } else {
                printf("Main produced: %s (%s)\n", item.name, item.team_name);
            }
        }

        Message ready_msg;
        while (msgrcv(mqid_ready, &ready_msg, sizeof(BakeryItem), 0, IPC_NOWAIT) >= 0) {
            BakeryItem *ready_item = &ready_msg.item;
            int placed = 0;

            for (int j = 0; j < config.NUM_OVENS; j++) {
                if (!game->ovens[j].is_busy) {
                    int baking_time = config.MIN_OVEN_TIME + rand() % (config.MAX_OVEN_TIME - config.MIN_OVEN_TIME + 1);
                    put_item_in_oven(&game->ovens[j], ready_item->name, ready_item->team_name, baking_time);
                    printf("Placed %s into Oven %d for %d seconds\n", ready_item->name, game->ovens[j].id, baking_time);
                    placed = 1;
                    break;
                }
            }

            if (!placed) {
                if (msgsnd(mqid_ready, &ready_msg, sizeof(BakeryItem), 0) == -1) {
                    perror("msgsnd requeue failed");
                }
                break;
            }
        }

        for (int j = 0; j < config.NUM_OVENS; j++) {
            if (oven_tick(&game->ovens[j])) {
                printf("Oven %d finished baking %s\n", game->ovens[j].id, game->ovens[j].item_name);
            }
        }

        sleep(1);
    }

    return 0;
}


pid_t start_process_baker(const char *binary, Config *config, int mqid_from_main, int mqid_ready) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        // Convert fd to string
        char mqid_from_main_str[10];
        char mqid_ready_str[10];
        char buffer[100];
        serialize_config(config, buffer);
        snprintf(mqid_from_main_str, sizeof(mqid_from_main_str), "%d", mqid_from_main);
        snprintf(mqid_ready_str, sizeof(mqid_ready_str), "%d", mqid_ready);

        if (execl(binary, binary, buffer, mqid_from_main_str, mqid_ready_str, NULL)) {

            printf("%s\n", binary);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}