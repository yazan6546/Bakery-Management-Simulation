#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include "chef.h"
#include "game.h"
#include <time.h>
#include <signal.h>

Game* game;
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <shm_fd> <msg_queue_id> <team>\n", argv[0]);
        exit(1);
    }

    // Parse arguments
    int fd = atoi(argv[1]);
    int msg_queue_id = atoi(argv[2]);
    ChefTeam team = atoi(argv[3]);

    // Map shared memory
    game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = move_chef;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    // Start chef work simulation
    simulate_chef_work(team, msg_queue_id, game);

    return 0;
}
