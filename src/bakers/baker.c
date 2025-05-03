/*  src/bakers/baker.c
 *  --------------------------------------------------------------
 *  ‑ one public input queue  (key = 0xCAFEBABE)
 *  ‑ three internal queues   (Bread / Cake‑Sweet / Patisserie)
 *  ‑ forks the required number of baker_worker processes
 *  ‑ forwards every incoming BakeryMessage to the correct team
 */
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
#include "semaphores_utils.h"

#define TEAM_COUNT          3
#define INPUT_QUEUE_KEY     0xCAFEBABE       /* public queue key    */

typedef struct {
    long         mtype;                      /* 1 bread, 2 cake, 3 patis */
    char         item_name[MAX_NAME_LENGTH];
    ProductType  category;
    int          index;                      /* index inside JSON list    */
} BakeryMessage;

/* product‑category → internal queue slot */
static inline int category_to_slot(ProductType cat)
{
    if (cat == BREAD)                        return 0; /* Bread team          */
    if (cat == CAKE || cat == SWEET)         return 1; /* Cake/Sweet team     */
    return 2;                                           /* Patisserie team    */
}

Game *game;                                  /* shared‑memory handle */

int main(void)
{
    /* ----- attach shared Game ------------------------------------ */
    int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); exit(EXIT_FAILURE); }

    game = mmap(NULL, sizeof(Game),
                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }
    close(shm_fd);

    printf("********** Bakery Simulation (manager) **********\n");
    print_config(&game->config);

    /* ----- create one Sys‑V queue per team ----------------------- */
    int team_q[TEAM_COUNT];
    for (int i = 0; i < TEAM_COUNT; ++i) {
        team_q[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        if (team_q[i] == -1) { perror("msgget team"); exit(EXIT_FAILURE); }
    }

    /* ----- fork baker_worker processes --------------------------- */
    BakerTeam teams[TEAM_COUNT];
    distribute_bakers_locally(&game->config, teams);

    for (int t = 0; t < TEAM_COUNT; ++t)
        for (int b = 0; b < teams[t].number_of_bakers; ++b) {
            if (fork() == 0) {
                char q_s[16], team_s[8];
                snprintf(q_s, sizeof q_s, "%d", team_q[t]);
                snprintf(team_s, sizeof team_s, "%d", teams[t].team_name);
                execl("./baker_worker", "baker_worker", q_s, team_s, NULL);
                perror("execl"); exit(EXIT_FAILURE);
            }
        }

    /* ----- public input queue (front‑door) ----------------------- */
    key_t in_key = INPUT_QUEUE_KEY;
    int   in_q   = msgget(in_key, 0666 | IPC_CREAT);
    if (in_q == -1) { perror("msgget input"); exit(EXIT_FAILURE); }

    printf("Manager ready – public queue key 0x%X  (id %d)\n",
           INPUT_QUEUE_KEY, in_q);
    puts("Send BakeryMessage structs here to feed the bakers…");

    /* ----- dispatcher loop --------------------------------------- */
    BakeryMessage msg;
    while (1) {
        if (msgrcv(in_q, &msg, sizeof msg - sizeof(long), 0, 0) == -1) {
            perror("manager msgrcv");
            continue;
        }

        int slot = category_to_slot(msg.category);
        if (msgsnd(team_q[slot], &msg, sizeof msg - sizeof(long), 0) == -1) {
            perror("manager msgsnd");
        } else {
            const char *team =
                (slot == 0) ? "Bread"
              : (slot == 1) ? "Cake/Sweet"
              :               "Patisserie";
            printf("→ dispatched %-20s to %s queue (id %d)\n",
                   msg.item_name, team, team_q[slot]);
        }
    }
    return 0;   /* never reached */
}
