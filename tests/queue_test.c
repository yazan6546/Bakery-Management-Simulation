#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "queue.h" // Assuming this contains the queue_shm declarations

typedef struct {
    int id;
    char name[32];
} Person;

int main() {
    // Create shared memory region
    const char* shm_name = "/my_queue_shm";
    size_t elemSize = sizeof(Person);
    size_t capacity = 10;
    size_t shm_size = queueShmSize(elemSize, capacity);

    // Create and map shared memory
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    // Set size of shared memory
    if (ftruncate(fd, shm_size) == -1) {
        perror("ftruncate failed");
        close(fd);
        shm_unlink(shm_name);
        return 1;
    }

    // Map the shared memory
    void* shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        shm_unlink(shm_name);
        return 1;
    }

    // Initialize queue in shared memory
    queue_shm* q = initQueueShm(shm_ptr, elemSize, capacity);

    // Use the queue
    Person p1 = {1, "Alice"};
    Person p2 = {2, "Bob"};
    Person p3 = {3, "Charlie"};

    // Enqueue elements
    printf("Adding people to the queue...\n");
    queueShmEnqueue(q, &p1);
    queueShmEnqueue(q, &p2);
    queueShmEnqueue(q, &p3);
    printf("Queue size: %zu\n", queueShmGetSize(q));

    // Peek at front element
    Person front_person;
    queueShmFront(q, &front_person);
    printf("Front person: %d, %s\n", front_person.id, front_person.name);

    // Remove an element by value
    Person to_remove = {2, "Bob"};
    printf("Removing Bob from the queue...\n");
    queueShmRemoveElement(q, &to_remove);
    printf("Queue size after removal: %zu\n", queueShmGetSize(q));

    // Dequeue and print elements
    printf("Dequeuing all elements:\n");
    Person p;
    while (!queueShmIsEmpty(q)) {
        queueShmDequeue(q, &p);
        printf("Dequeued: %d, %s\n", p.id, p.name);
    }

    // Create a child process to demonstrate shared memory access
    pid_t pid = fork();

    if (pid == 0) { // Child process
        printf("Child process adding new elements\n");

        Person c1 = {10, "David"};
        Person c2 = {11, "Emma"};

        queueShmEnqueue(q, &c1);
        queueShmEnqueue(q, &c2);

        printf("Child process finished, queue size: %zu\n", queueShmGetSize(q));
        exit(0);
    } else { // Parent process
        // Wait for child to complete
        sleep(1);

        printf("Parent process reading queue, size: %zu\n", queueShmGetSize(q));

        // Dequeue elements added by child
        while (!queueShmIsEmpty(q)) {
            queueShmDequeue(q, &p);
            printf("Parent dequeued: %d, %s\n", p.id, p.name);
        }
    }

    // Clean up
    munmap(shm_ptr, shm_size);
    close(fd);
    shm_unlink(shm_name);

    return 0;
}