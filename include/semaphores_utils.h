#ifndef SEMAPHORES_UTILS_H
#define SEMAPHORES_UTILS_H
#include <semaphore.h>

#define SEM_NAME "/bakery_inventory_sem"
#define READY_SEM_NAME "/bakery_ready_products_sem"
#define COMPLAINT_SEM_NAME "/bakery_complaint_sem"


sem_t* setup_inventory_semaphore(void);
void lock_inventory(sem_t* sem);
void unlock_inventory(sem_t* sem);
void cleanup_inventory_semaphore_resources(sem_t* inventory_sem);
// Function prototypes for ready products
sem_t* setup_ready_products_semaphore(void);
void lock_ready_products(sem_t* sem);
void unlock_ready_products(sem_t* sem);
void cleanup_ready_products_semaphore_resources(sem_t* ready_products_sem);

#endif // SEMAPHORES_UTILS_H