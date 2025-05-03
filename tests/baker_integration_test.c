/*
 * baker_debug_suite.c
 *
 * Spawns one bread‑team baker_worker, feeds multiple edge‑case
 * messages, and prints a timeline.  Exits 0 when all checks pass.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "oven.h"
#include "inventory.h"
#include "semaphores_utils.h"
#include "products.h"
#include "BakerTeam.h"

/* ---------- helpers & defs ---------- */
#define SHM_NAME "/game_shared_mem"
#define dbg(fmt, ...)  printf("t=%-3lds  " fmt "\n", time(NULL)-t0, ##__VA_ARGS__)

typedef struct {
    long         mtype;
    char         item_name[MAX_NAME_LENGTH];
    ProductType  category;
    int          index;
} BakeryMessage;

static time_t t0;

/* minimal deterministic config */
static void tiny_config(Config *c)
{
    memset(c, 0, sizeof(*c));
    c->NUM_OVENS      = 1;
    c->MIN_BAKE_TIME  = 1;
    c->MAX_BAKE_TIME  = 1;
    c->MIN_OVEN_TIME  = 2;      /* oven busy a bit longer */
    c->MAX_OVEN_TIME  = 2;
    c->NUM_BAKERS     = 1;
}

static int queue_depth(int mqid)
{
    struct msqid_ds ds;
    msgctl(mqid, IPC_STAT, &ds);
    return ds.msg_qnum;
}

/* ---------- main test ---------- */
int main(void)
{
    t0 = time(NULL);

    /* shared Game */
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Game));
    Game *game = mmap(NULL, sizeof(Game),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    tiny_config(&game->config);
    init_inventory(&game->inventory);
    init_ready_products(&game->ready_products);
    init_oven(&game->ovens[0], 0);
    setup_ready_products_semaphore();
    setup_oven_semaphores(1);

    /* queue + worker */
    int mqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    pid_t worker = fork();
    if (worker == 0) {
        char mqid_s[16], team_s[8];
        snprintf(mqid_s, sizeof(mqid_s), "%d", mqid);
        snprintf(team_s, sizeof(team_s), "%d", BREAD_BAKERS);
        execl("./baker_worker", "baker_worker", mqid_s, team_s, NULL);
        perror("execl"); exit(1);
    }

    /* ---- CASE 1 & 2 : enqueue items ---- */
    BakeryMessage m = {.mtype = 1, .category = BREAD, .index = 0 };
    strcpy(m.item_name,"White Bread");
    msgsnd(mqid,&m,sizeof m - sizeof(long),0);  dbg("SEND bread[0]");

    sleep(1);
    strcpy(m.item_name,"Baguette");             m.index = 1;
    msgsnd(mqid,&m,sizeof m - sizeof(long),0);  dbg("SEND bread[1]");

    sleep(1);                                  /* wrong team */
    m.category = CAKE; m.mtype = 2; strcpy(m.item_name,"Chocolate Cake");
    msgsnd(mqid,&m,sizeof m - sizeof(long),0);  dbg("SEND wrong‑team CAKE");

    /* ---- observe timeline for ~10 s ---- */
    int pass_ready = 0, pass_wrong = 0, pass_order = 0;
    int ready0 = 0, ready1 = 0;
    for (int sec = 0; sec < 12; ++sec) {
        sleep(1);
        ready0 = game->ready_products.categories[BREAD].quantities[0];
        ready1 = game->ready_products.categories[BREAD].quantities[1];

        if (!pass_ready && ready0 == 1 && ready1 == 1)
            pass_ready = 1;

        if (!pass_wrong && queue_depth(mqid) == 1)   /* cake left over  */
            pass_wrong = 1;

        /* order preserved: ready0 must appear first */
        if (!pass_order && ready0 == 1 && ready1 == 0)
            pass_order = 1;

        dbg("status  ready0=%d  ready1=%d  qdepth=%d",
            ready0, ready1, queue_depth(mqid));
    }

    int ok = pass_ready && pass_wrong && pass_order;
    printf("\nRESULT  ready++ %s, wrong‑team %s, order %s  =>  %s\n",
        pass_ready? "OK":"FAIL",
        pass_wrong? "OK":"FAIL",
        pass_order? "OK":"FAIL",
        ok? "PASS":"FAIL");

    /* cleanup */
    kill(worker,SIGTERM);
    waitpid(worker,NULL,0);
    msgctl(mqid,IPC_RMID,NULL);
    munmap(game,sizeof(Game));
    shm_unlink(SHM_NAME);
    cleanup_oven_semaphores(1);
    return ok?0:1;
}
