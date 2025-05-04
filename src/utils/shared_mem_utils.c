#include "shared_mem_utils.h"
#include <fcntl.h>
#include <sys/mman.h>
#include "customer.h"
#include "queue.h"



int setup_shared_memory(Game **shared_game) {
    // Setup game shared memory as before
    int shm_fd = shm_open(GAME_SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Game));
    *shared_game = mmap(0, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (*shared_game == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }

    fcntl(shm_fd, F_SETFD, fcntl(shm_fd, F_GETFD) & ~FD_CLOEXEC);
    if (*shared_game == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }

    return shm_fd;
}

void cleanup_shared_memory(Game *shared_game) {
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
    shm_unlink(GAME_SHM_NAME);
}

void cleanup_queue_shared_memory(queue_shm *queue_shm, size_t capacity) {
    size_t elemSize = sizeof(Customer);
    size_t shm_size = queueShmSize(elemSize, capacity);

    if (queue_shm != NULL && queue_shm != MAP_FAILED) {
        if (munmap(queue_shm, shm_size) == -1) {
            perror("munmap failed");
        }
    }
    shm_unlink(CUSTOMER_QUEUE_SHM_NAME);
}

void setup_queue_shared_memory(queue_shm **queue_shm, size_t capacity) {
    size_t elemSize = sizeof(Customer);
    size_t shm_size = queueShmSize(elemSize, capacity);

    int queue_fd = shm_open(CUSTOMER_QUEUE_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (queue_fd == -1) {
        perror("Failed to open queue shared memory");
        exit(EXIT_FAILURE);
    }
    ftruncate(queue_fd, (long) shm_size);
    *queue_shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, queue_fd, 0);

    if (*queue_shm == MAP_FAILED) {
        perror("Failed to map queue shared memory");
        exit(EXIT_FAILURE);
    }

    fcntl(queue_fd, F_SETFD, fcntl(queue_fd, F_GETFD) & ~FD_CLOEXEC);
    close(queue_fd);
}