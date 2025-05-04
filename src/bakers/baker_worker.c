#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>

#include "oven.h"
#include "inventory.h"
#include "game.h"
#include "BakerTeam.h"
#include "bakery_utils.h"
#include "products.h"
#include "semaphores_utils.h"

#define MAX_NAME MAX_NAME_LENGTH

typedef struct {
    long         mtype;
    char         item_name[MAX_NAME];
    ProductType  category;
    int          index;
} BakeryMessage;

typedef enum { IDLE, BUSY } BakerState;

static inline Team category_to_team(ProductType c)
{
    switch (c) {
        case BREAD:  return BREAD_BAKERS;
        case CAKE:
        case SWEET:  return CAKE_AND_SWEETS_BAKERS;
        default:     return PASTRIES_BAKERS;
    }
}

Game *game;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mqid> <team_enum>\n", argv[0]);
        return EXIT_FAILURE;
    }


    // Parse arguments
    // Convert message queue ID and team enum from string to integer
    int  mqid    = atoi(argv[1]);
    Team my_team = (Team)atoi(argv[2]);

    // Open shared memory
    int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); exit(EXIT_FAILURE); }
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }
    close(shm_fd);

    // Setup semaphores
    sem_t *ready_sem = setup_ready_products_semaphore();
    setup_oven_semaphores(game->config.NUM_OVENS);

    
    BakerState    state    = IDLE;
    BakeryMessage cur_msg  = {0};
    int           oven_idx = -1;

    while (1) {

        if (state == BUSY && oven_idx != -1) {
            oven_tick(&game->ovens[oven_idx]);
            if (!game->ovens[oven_idx].is_busy) {
                add_ready_product(&game->ready_products,
                                  cur_msg.category,
                                  cur_msg.index,
                                  1,
                                  ready_sem);
                printf("[Baker %s] Finished %s in oven %d\n",
                       get_team_name_str(my_team),
                       cur_msg.item_name, oven_idx);
                state = IDLE;
                oven_idx = -1;
            } else {
                sleep(1);
                continue;
            }
        }

        if (state == IDLE) {
            BakeryMessage msg;
            ssize_t r = msgrcv(mqid, &msg, sizeof(BakeryMessage) - sizeof(long), 0, 0);
            if (r < 0) {
                if (errno == EIDRM || errno == EINVAL) _exit(0);
                if (errno == EINTR) continue;
                perror("[worker] msgrcv");
                continue;
            }

            if (category_to_team(msg.category) != my_team) {
                msgsnd(mqid, &msg, sizeof(BakeryMessage) - sizeof(long), 0);
                usleep(10000);
                continue;
            }

            cur_msg = msg;

            int prep = game->config.MIN_BAKE_TIME +
                       rand() % (game->config.MAX_BAKE_TIME - game->config.MIN_BAKE_TIME + 1);
            printf("[Baker %s] Preparing %s (%d s)\n",
                   get_team_name_str(my_team),
                   cur_msg.item_name, prep);
            sleep(prep);
            printf("[Baker %s] %s finished preparation\n",
                   get_team_name_str(my_team), cur_msg.item_name);

            while (1) {
                for (int i = 0; i < game->config.NUM_OVENS; ++i) {
                    int bake_time = game->config.MIN_OVEN_TIME +
                                    rand() % (game->config.MAX_OVEN_TIME - game->config.MIN_OVEN_TIME + 1);
                    if (put_item_in_oven(&game->ovens[i],
                                         cur_msg.item_name,
                                         get_team_name_str(my_team),
                                         bake_time)) {
                        oven_idx = i;
                        state    = BUSY;
                        printf("[Baker %s] Placed %s in oven %d for %d s\n",
                               get_team_name_str(my_team),
                               cur_msg.item_name, i, bake_time);
                        goto next_cycle;
                    }
                }
                printf("[Baker %s] No oven free – %s keeps waiting\n",
                       get_team_name_str(my_team), cur_msg.item_name);
                sleep(1);
                for (int i = 0; i < game->config.NUM_OVENS; ++i)
                    oven_tick(&game->ovens[i]);
            }
        }
next_cycle:
        continue;
    }
    return 0;
}
