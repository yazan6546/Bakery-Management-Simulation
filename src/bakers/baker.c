
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <string.h>
#include <fcntl.h>

#include "oven.h"
#include "BakerTeam.h"
#include "game.h"
#include "bakery_utils.h"
#include "products.h"
#include "semaphores_utils.h"    /* optional: if manager ever uses them */

#define TEAM_COUNT 3

typedef struct {
    long         mtype;                       /* 1 bread, 2 cake/sweet, 3 patis. */
    char         item_name[MAX_NAME_LENGTH];
    ProductType  category;
    int          index;                       /* index in JSON list */
} BakeryMessage;

Game *game;

int main(void)
{
    int fd = shm_open("/game_shared_mem", O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); exit(EXIT_FAILURE); }

    game = mmap(NULL, sizeof(Game),
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (game == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }
    close(fd);

    printf("********** Bakery Simulation (manager) **********\n");
    print_config(&game->config);

    /* one Sys‑V queue per team */
    int mqids[TEAM_COUNT];
    for (int i = 0; i < TEAM_COUNT; ++i) {
        mqids[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        if (mqids[i] == -1) { perror("msgget"); exit(EXIT_FAILURE); }
    }

    /* fork a worker for every baker */
    BakerTeam teams[TEAM_COUNT];
    distribute_bakers_locally(&game->config, teams);

    for (int t = 0; t < TEAM_COUNT; ++t)
        for (int b = 0; b < teams[t].number_of_bakers; ++b) {
            if (fork() == 0) {
                char mqid_str[16], team_str[8];
                snprintf(mqid_str, sizeof(mqid_str), "%d", mqids[t]);
                snprintf(team_str, sizeof(team_str), "%d", teams[t].team_name);
                execl("./baker_worker", "baker_worker",
                      mqid_str, team_str, NULL);
                perror("execl"); exit(EXIT_FAILURE);
            }
        }

   // pause();
    return 0;
}
