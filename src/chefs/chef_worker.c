#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include "chef.h"
#include "game.h"
#include <time.h>
#include <signal.h>
#include "shared_mem_utils.h"
#include "team.h"

Game* game;
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <msg_queue_id> <team>\n", argv[0]);
        exit(1);
    }

    setup_shared_memory(&game);


    // Parse arguments
    int msg_queue_id = atoi(argv[1]);
    ChefTeam team = atoi(argv[2]);
    int id = atoi(argv[3]);

    if(argc != 4) {
        fprintf(stderr, "Usage: %s <msg_queue_id> <team> <id>\n", argv[0]);
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = move_chef;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    // Start chef work simulation
    simulate_chef_work(team, msg_queue_id, game, id);

    return 0;
}
