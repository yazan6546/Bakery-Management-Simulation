#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdlib.h>

/**
 * Queue structure for shared memory implementation
 * Note: The structure is defined here so clients know its memory layout
 */
typedef struct {
    size_t head;           // Index of the head element
    size_t tail;           // Index of the tail element
    size_t count;          // Number of elements in the queue
    size_t elemSize;       // Size of each element
    size_t capacity;       // Maximum number of elements
    char elements[1];      // Flexible array member (actual size will be capacity * elemSize)
} queue_shm;

/**
 * Calculate the total size needed for the queue in shared memory
 * @param elemSize Size of each element in bytes
 * @param capacity Maximum number of elements
 * @return Total size required in bytes
 */
size_t queueShmSize(size_t elemSize, size_t capacity);

/**
 * Initialize a queue in an existing shared memory block
 * @param memory Pointer to allocated shared memory
 * @param elemSize Size of each element in bytes
 * @param capacity Maximum number of elements
 * @return Pointer to the initialized queue or NULL on error
 */
queue_shm* initQueueShm(void* memory, size_t elemSize, size_t capacity);

/**
 * Add an element to the end of the queue
 * @param q Pointer to the queue
 * @param data Pointer to the data to enqueue
 * @return 0 on success, -1 on failure
 */
int queueShmEnqueue(queue_shm* q, const void* data);

/**
 * Remove an element from the front of the queue
 * @param q Pointer to the queue
 * @param data Pointer to store the dequeued data
 * @return 0 on success, -1 on failure
 */
int queueShmDequeue(queue_shm* q, void* data);

/**
 * Get the front element without removing it
 * @param q Pointer to the queue
 * @param data Pointer to store the front element data
 * @return 0 on success, -1 on failure
 */
int queueShmFront(queue_shm* q, void* data);

/**
 * Check if the queue is empty
 * @param q Pointer to the queue
 * @return 1 if empty, 0 otherwise
 */
int queueShmIsEmpty(queue_shm* q);

/**
 * Check if the queue is full
 * @param q Pointer to the queue
 * @return 1 if full, 0 otherwise
 */
int queueShmIsFull(queue_shm* q);

/**
 * Get the number of elements in the queue
 * @param q Pointer to the queue
 * @return Number of elements in the queue
 */
size_t queueShmGetSize(queue_shm* q);

/**
 * Clear all elements from the queue
 * @param q Pointer to the queue
 */
void queueShmClear(queue_shm* q);

/**
 * Remove an element from the queue based on content
 * @param q Pointer to the queue
 * @param data Pointer to data to match and remove
 * @return 0 on success, -1 if element not found or error
 */
int queueShmRemoveElement(queue_shm* q, const void* data);

/**
 * Remove an element from the queue at a specified index
 * @param q Pointer to the queue
 * @param index Index of the element to remove (0-based, from front of queue)
 * @return 0 on success, -1 if index is invalid or error
 */
int queueShmRemoveAt(queue_shm* q, size_t index);
#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H */