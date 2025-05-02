#include "shared_mem_utils.h"
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/game_shared_mem"


void setup_shared_memory(Game **shared_game) {
    // Setup game shared memory as before
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Game));
    *shared_game = mmap(0, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    fcntl(shm_fd, F_SETFD, fcntl(shm_fd, F_GETFD) & ~FD_CLOEXEC);
    close(shm_fd);
    if (*shared_game == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }
}

void cleanup_shared_memory(Game *shared_game) {
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
    shm_unlink(SHM_NAME);
}