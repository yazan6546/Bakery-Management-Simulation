#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "customer.h"
#include "shared_mem_utils.h"
#include "semaphores_utils.h"

#include "queue.h" // Assuming this contains the queue_shm declarations

int main() {
    // Create shared memory region
    size_t elemSize = sizeof(Customer);
    size_t capacity = 10;
    size_t shm_size = queueShmSize(elemSize, capacity);

    queue_shm *customer_queue;
    setup_queue_shared_memory(&customer_queue, 10);

    sem_t *sem = sem_open(QUEUE_SEM_NAME, O_CREAT, 0666, 1);

    sem_wait(sem);
    for (int i = 0; i < customer_queue->count; i++) {
        int j = (customer_queue->head + i) % customer_queue->capacity;
        Customer *c = &((Customer*)customer_queue->elements)[j];

        printf("PID %d\n", c->pid);
    }

    sem_post(sem);
    // Clean up
    munmap(customer_queue, shm_size);

    return 0;
}