//
// Created by yazan on 5/4/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

// Define a structure with a flexible array member
typedef struct {
    int count;       // Number of elements in the array
    int chef_count;  // For demonstration of other fields
    char data[];     // Flexible array member - MUST be last field
} SharedData;

#define SHM_NAME "/flex_array_test"

int main() {
    int shm_fd;
    SharedData *shared_data;
    pid_t pid;
    int array_size = 10;  // Size we want for the flexible array
    size_t total_size = sizeof(SharedData) + (array_size * sizeof(char));

    // Create and configure shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // Set the size of shared memory segment
    if (ftruncate(shm_fd, total_size) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    // Map shared memory
    shared_data = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Initialize shared memory
    shared_data->count = array_size;
    shared_data->chef_count = 5;  // Just a demo value

    // Fork to test shared memory across processes
    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Child process
        // Child writes to the flexible array
        printf("Child: Writing to flexible array\n");
        for (int i = 0; i < shared_data->count; i++) {
            shared_data->data[i] = 'A' + i;
        }
        printf("Child: Write complete\n");
        exit(EXIT_SUCCESS);
    } else {  // Parent process
        // Wait for child to complete
        waitpid(pid, NULL, 0);

        // Parent reads from the flexible array
        printf("Parent: Reading from flexible array\n");
        printf("Chef count: %d\n", shared_data->chef_count);
        printf("Array content: ");
        for (int i = 0; i < shared_data->count; i++) {
            printf("%c ", shared_data->data[i]);
        }
        printf("\n");

        // Clean up shared memory
        munmap(shared_data, total_size);
        shm_unlink(SHM_NAME);
    }

    return 0;
}