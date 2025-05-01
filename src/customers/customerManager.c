#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "customer.h"
#include "game.h"
#include "queue.h"
#include "random.h"


void spawn_customer(Config *config, queue_shm *customerQueue, int msgQueueID, int custID) {
    Customer newCust;
    newCust.patience = random_float(config->MIN_PATIENCE, config->MAX_PATIENCE);
    newCust.has_complained = false;
    newCust.state = WALKING;
    queueShmEnqueue(customerQueue, &newCust);

    pid_t pid = fork();
    if (pid == 0) {
        char id_buf[8];
        char cust_buf[8];
        snprintf(id_buf, sizeof(id_buf), "%d", msgQueueID);
        snprintf(cust_buf, sizeof(cust_buf), "%d", custID);
        if (execl("./customer", "./customer", id_buf, cust_buf, NULL) == -1) {

            perror("execl failed");
            exit(EXIT_FAILURE);
        }        
    }
}

void handle_queue(queue_shm *q) {
    size_t size = queueShmGetSize(q);
    for (size_t i = 0; i < size; ++i) {
        Customer temp;
        queueShmDequeue(q, &temp);

        int complaintRisk = rand() % 10;
        if (shared_game->num_complained_customers >= shared_game->config.COMPLAINED_CUSTOMERS &&
            complaintRisk < 5) {
            printf("Customer %d saw too many complaints and left.\n", temp.id);
            shared_game->num_customers_missing++;
            continue;
        }

        int waitTime = rand() % 10 + 1;
        if (waitTime > temp.patience) {
            printf("Customer %d got frustrated while waiting and left.\n", temp.id);
            shared_game->num_frustrated_customers++;
            continue;
        }

        enqueue(q, &temp); // Keep in queue if didn't leave
    }
}

int main() {
    srand(time(NULL));
    setup_shared_memory();

    queue *customerQueue = createQueue(sizeof(Customer));
    int msgQueueID = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgQueueID == -1) {
        perror("msgget");
        exit(1);
    }

    for (int i = 0; i < shared_game->config.MAX_CUSTOMERS; i++) {
        sleep(rand() % 3 + 2);
        spawn_customer(customerQueue, msgQueueID, i + 1);
        handle_queue(customerQueue);
    }

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        wait(NULL);
    }

    printf("\nFinal Game State:\n");
    printf("Complaints: %d\n", shared_game->num_complained_customers);
    printf("Frustrated & left: %d\n", shared_game->num_frustrated_customers);
    printf("Left while queueing due to complaints: %d\n", shared_game->num_customers_missing);

    destroyQueue(&customerQueue);
    msgctl(msgQueueID, IPC_RMID, NULL);
    shm_unlink("/game_shm");

    return 0;
}
