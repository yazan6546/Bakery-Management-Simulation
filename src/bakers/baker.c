#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>

#include "queue.h"
#include "oven.h"
#include "BakeryItem.h"
#include "BakerTeam.h"
#include "game.h"
#include "random.h"
#include "bakery_utils.h"

typedef struct {
    long mtype;
    BakeryItem item;
} Message;

Game *game;

int main(int argc, char *argv[]) {
    int fd = shm_open("/game_shm", O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    close(fd);

    printf("********** Bakery Simulation **********\n");
    Config config = game->config;
    print_config(&config);
    init_random();

    for (int i = 0; i < config.NUM_OVENS; i++) {
        init_oven(&game->ovens[i], i);
    }

    int mqid_bread = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    int mqid_cakesweets = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    int mqid_patisseries = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    if (mqid_bread == -1 || mqid_cakesweets == -1 || mqid_patisseries == -1) {
        perror("msgget failed");
        exit(1);
    }

    BakerTeam teams[3];
    distribute_bakers_locally(&config, teams);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < teams[i].number_of_bakers; j++) {
            pid_t pid = fork();
            if (pid == 0) {
                char mqid_str[16], team_str[8];
                int mqid = (teams[i].team_name == BREAD_BAKERS) ? mqid_bread :
                           (teams[i].team_name == CAKE_AND_SWEETS_BAKERS) ? mqid_cakesweets :
                           mqid_patisseries;

                snprintf(mqid_str, sizeof(mqid_str), "%d", mqid);
                snprintf(team_str, sizeof(team_str), "%d", teams[i].team_name);

                execl("./baker_worker", "baker_worker", mqid_str, team_str, NULL);
                perror("execl failed");
                exit(1);
            }
        }
    }

    int t = 0;
    while (1) {
        printf("\n=== Time Step %d ===\n", ++t);
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

            Message msg = {1, item};

            int target_mqid = (team == 0) ? mqid_bread :
                              (team == 1) ? mqid_cakesweets : mqid_patisseries;

            if (msgsnd(target_mqid, &msg, sizeof(BakeryItem), 0) == -1) {
                perror("msgsnd failed");
            } else {
                printf("Produced: %s (%s)\n", item.name, item.team_name);
            }
        }

        sleep(1);
    }

    return 0;
}
