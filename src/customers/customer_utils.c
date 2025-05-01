//
// Created by yazan on 4/30/2025.
//

#include "customer.h"
#include "config.h"
#include "game.h"
#include "random.h"
#include "queue.h"

void create_random_customer(Customer *customer, Config *config) {

    if (!customer)
        return;
    customer->patience = random_float(config->MIN_PATIENCE, config->MAX_PATIENCE);
    customer->patience_decay -= random_float(config->MIN_PATIENCE_DECAY, config->MAX_PATIENCE_DECAY);
    customer->has_complained = false;
    customer->state = WALKING;
}

void serialize_customer(Customer *customer, char *buffer) {
    sprintf (buffer, "%f %f %d %d",
            customer->patience,
            customer->patience_decay,
            customer->has_complained,
            customer->state);
}

void deserialize_customer(Customer *customer, char *buffer) {
    sscanf (buffer, "%f %f %d %d",
            &customer->patience,
            &customer->patience_decay,
            &customer->has_complained,
            &customer->state);
}
void free_customer(Customer *customer) {
    if (customer) {
        free(customer);
    }
}

void setup_shared_memory(queue_shm **customer_queue, Game **shared_game) {
    // Setup game shared memory as before
    int shm_fd = shm_open("/game_shared_mem", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Game));
    *shared_game = mmap(0, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    if (shared_game == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }
    fcntl(shm_fd, F_SETFD, fcntl(shm_fd, F_GETFD) & ~FD_CLOEXEC);


    // Setup queue shared memory
    const char* queue_shm_name = "/customer_queue_shm";
    size_t elemSize = sizeof(Customer);
    size_t capacity = (*shared_game)->config.MAX_CUSTOMERS;
    size_t shm_size = queueShmSize(elemSize, capacity);

    int queue_fd = shm_open(queue_shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(queue_fd, (long) shm_size);
    void* queue_shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, queue_fd, 0);
    fcntl(queue_fd, F_SETFD, fcntl(queue_fd, F_GETFD) & ~FD_CLOEXEC);
    close(queue_fd);

    // Initialize queue in shared memory
    *customer_queue = initQueueShm(queue_shm_ptr, elemSize, capacity);
}

void print_customer(Customer *customer) {
    printf("Customer: %f %f %d %d\n", customer->patience, customer->patience_decay, customer->has_complained, customer->state);

}