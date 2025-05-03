#include "oven.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <semaphore.h>


#define SEM_OVEN_NAME_BASE "/oven_sem_"
#define MAX_NUM_OVENS 10

sem_t *oven_sems[MAX_NUM_OVENS];

// Generate a unique semaphore name for each oven
void get_oven_sem_name(int oven_id, char *buffer, size_t size) {
    snprintf(buffer, size, "%s%d", SEM_OVEN_NAME_BASE, oven_id);
}

// Initialize oven struct
void init_oven(Oven *oven, int id) {
    oven->id = id;
    oven->is_busy = 0;
    oven->time_left = 0;
    oven->item_name[0] = '\0';
    oven->team_name[0] = '\0';
}

// Setup semaphore for each oven
int setup_oven_semaphores(int num_ovens) {
    for (int i = 0; i < num_ovens; i++) {
        char sem_name[64];
        get_oven_sem_name(i, sem_name, sizeof(sem_name));
        oven_sems[i] = sem_open(sem_name, O_CREAT, 0666, 1);
        if (oven_sems[i] == SEM_FAILED) {
            perror("sem_open for oven failed");
            return -1;
        }
    }
    return 0;
}

// Lock specific oven
void lock_oven(int oven_id) {
    if (sem_wait(oven_sems[oven_id]) == -1) {
        perror("sem_wait failed on oven");
        exit(1);
    }
}

// Unlock specific oven
void unlock_oven(int oven_id) {
    if (sem_post(oven_sems[oven_id]) == -1) {
        perror("sem_post failed on oven");
        exit(1);
    }
}

// Place an item in the oven (with locking)
int put_item_in_oven(Oven *oven, const char *item_name, const char *team_name, int baking_time) {
    lock_oven(oven->id);

    if (oven->is_busy) {
        unlock_oven(oven->id);
        return 0;
    }

    oven->is_busy = 1;
    oven->time_left = baking_time;
    strcpy(oven->item_name, item_name);
    strcpy(oven->team_name, team_name);

    unlock_oven(oven->id);
    return 1;
}

// Tick oven time (with locking)
int oven_tick(Oven *oven) {
    int finished = 0;

    lock_oven(oven->id);

    if (oven->is_busy) {
        oven->time_left--;
        if (oven->time_left <= 0) {
            oven->is_busy = 0;
            printf("Oven %d finished baking %s\n", oven->id, oven->item_name);
            oven->item_name[0] = '\0';
            oven->team_name[0] = '\0';
            finished = 1;
        }
    }

    unlock_oven(oven->id);
    return finished;
}

void cleanup_oven_semaphores(int num_ovens) {
    for (int i = 0; i < num_ovens; i++) {
        char sem_name[64];
        get_oven_sem_name(i, sem_name, sizeof(sem_name));
        sem_close(oven_sems[i]);
        sem_unlink(sem_name);
    }
}
