#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "assets.h"
#include "config.h"
#include "game.h"
#include "queue.h"
#include "shared_mem_utils.h"
#include "semaphores_utils.h"

/* globals from your original code --------------------------- */
Game  *shared_game         = NULL;
pid_t  processes[6];
pid_t *processes_sellers   = NULL;
int    shm_fd              = -1;
queue_shm *queue           = NULL;

/* ---- NEW: tick all ovens once per real‑time second -------- */
static void tick_all_ovens(Game *g)
{
    for (int i = 0; i < g->config.NUM_OVENS; ++i) {
        Oven *ov = &g->ovens[i];
        if (!ov->is_busy) continue;

        if (--ov->time_left <= 0) {
            ov->is_busy = 0;
            printf("[main] Oven %d finished baking %s (team %s)\n",
                   ov->id, ov->item_name, ov->team_name);

            /* clear the text fields (same as old oven_tick) */
            ov->item_name[0] = '\0';
            ov->team_name[0] = '\0';
        }
    }
}

/* ----------------------------------------------------------- */
void handle_alarm(int signum)
{
    shared_game->elapsed_time++;   /* original work            */
    tick_all_ovens(shared_game);   /* NEW: simulate ovens      */
    alarm(1);                      /* re‑arm                   */
}

void cleanup_resources(void);
void handle_kill(int);

int main(int argc,char *argv[])
{
    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);

    reset_all_semaphores();

    atexit(cleanup_resources);

    shm_fd = setup_shared_memory(&shared_game);
    setup_queue_shared_memory(&queue,shared_game->config.MAX_CUSTOMERS);

    processes_sellers = malloc(shared_game->config.NUM_SELLERS*sizeof(pid_t));

    signal(SIGALRM,handle_alarm);
    signal(SIGINT ,handle_kill);

    if (load_config(CONFIG_PATH,&shared_game->config)==-1){
        printf("Config file failed\n"); return 1;
    }
    if (load_product_catalog(CONFIG_PATH_JSON,&shared_game->productCatalog)==-1){
        printf("Product catalog file failed\n"); return 1;
    }

    game_init(shared_game,processes,processes_sellers,shm_fd);

    alarm(1);               /* start 1‑second timer */

    /* empty polling loop – stays as before */
    while (check_game_conditions(shared_game)){ /* nothing */ }

    /* wait for graphics process (index 0 in your array) */
    int status_graphics;
    waitpid(processes[0], &status_graphics, 0);

    if (WIFEXITED(status_graphics))
        printf("Graphics child exited with code %d\n", WEXITSTATUS(status_graphics));
    else if (WIFSIGNALED(status_graphics))
        printf("Graphics child killed by signal %d\n", WTERMSIG(status_graphics));

    return 0;  /* cleanup_resources is run automatically */
}

/* ---- unchanged cleanup / signal handlers ------------------ */
void cleanup_resources()
{
    printf("Cleaning up resources...\n"); fflush(stdout);

    for(int i=0;i<6;i++) kill(processes[i],SIGINT);
    cleanup_shared_memory(shared_game);
    shm_unlink(CUSTOMER_QUEUE_SHM_NAME);
    free(processes_sellers);
    printf("Cleanup complete\n");
}
void handle_kill(int signum){ exit(0); }
