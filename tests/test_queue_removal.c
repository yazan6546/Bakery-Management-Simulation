//
// Created by yazan on 5/2/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "customer.h"

void test_queue_removal(queue_shm *test_queue) {
    printf("===== QUEUE REMOVAL TEST =====\n");

    // Create a test queue with 5 customers
    Customer test_customers[5];
    for (int i = 0; i < 5; i++) {
        test_customers[i].id = i + 100;
        test_customers[i].pid = 10000 + i;
        test_customers[i].state = WAITING_IN_QUEUE;
        test_customers[i].patience = 5.0f - i;
    }

    // Initialize the queue

    // Add customers to queue
    for (int i = 0; i < 5; i++) {
        queueShmEnqueue(test_queue, &test_customers[i]);
    }

    // Print initial queue state
    printf("Initial queue (count=%d):\n", test_queue->count);
    for (int i = 0; i < test_queue->count; i++) {
        Customer* c = &((Customer*)test_queue->elements)[i];
        printf("Position %d: ID=%d, PID=%d, patience=%.2f\n",
               i, c->id, c->pid, c->patience);
    }

    // Remove element from middle (position 2)
    int remove_pos = 2;
    printf("\nRemoving element at position %d\n", remove_pos);
    queueShmRemoveAt(test_queue, remove_pos);

    // Print queue after removal
    printf("\nQueue after removal (count=%d):\n", test_queue->count);
    for (int i = 0; i < test_queue->count; i++) {
        Customer* c = &((Customer*)test_queue->elements)[i];
        printf("Position %d: ID=%d, PID=%d, patience=%.2f\n",
               i, c->id, c->pid, c->patience);
    }

    // Try removing first element
    printf("\nRemoving element at position 0\n");
    queueShmRemoveAt(test_queue, 0);

    // Print queue after removal
    printf("\nQueue after removing first element (count=%zu):\n", test_queue->count);
    for (int i = 0; i < test_queue->count; i++) {
        Customer* c = &((Customer*)test_queue->elements)[i];
        printf("Position %d: ID=%d, PID=%d, patience=%.2f\n",
               i, c->id, c->pid, c->patience);
    }

    // Try removing last element
    int last_pos = test_queue->count - 1;
    printf("\nRemoving element at position %d (last)\n", last_pos);
    queueShmRemoveAt(test_queue, last_pos);

    // Print queue after removal
    printf("\nQueue after removing last element (count=%zu):\n", test_queue->count);
    for (int i = 0; i < test_queue->count; i++) {
        Customer* c = &((Customer*)test_queue->elements)[i];
        printf("Position %d: ID=%d, PID=%d, patience=%.2f\n",
               i, c->id, c->pid, c->patience);
    }

    // Clean up
    printf("===== TEST COMPLETE =====\n\n");
}

int main() {

    queue_shm *test_queue;
    Game *shared_game;
    setup_shared_memory(&test_queue, &shared_game);
    test_queue_removal(test_queue);

}