#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "customer.h"
#include "game.h"

struct msgbuf {
    long mtype;
    char mtext[128];
};

int main(int argc, char *argv[]) {
    if (argc < 3) exit(EXIT_FAILURE);

    int msgID = atoi(argv[1]);
    int custID = atoi(argv[2]);

    // Attach to shared memory (Game)
    int shm_fd = shm_open("/game_shm", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    Game *shared_game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_game == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    srand(time(NULL) ^ (custID << 8)); // Better randomness

    Customer customer;
    customer.id = custID;
    customer.patience = rand() % 10 + 5;
    customer.has_complained = 0;
    customer.state = WALKING;

    int waitTime = rand() % 10 + 1;

    while (1) {
        switch (customer.state) {
            case WALKING:
                printf("Customer %d is walking...\n", customer.id);
                sleep(rand() % 3);
                customer.state = WAITING_IN_QUEUE;
                break;

            case WAITING_IN_QUEUE:
                printf("Customer %d is waiting in queue with patience %d...\n", customer.id, customer.patience);
                customer.state = ORDERING;
                break;

            case ORDERING: {
                struct msgbuf orderMsg;
                orderMsg.mtype = 1;
                snprintf(orderMsg.mtext, sizeof(orderMsg.mtext), "Customer %d ordering", customer.id);
                msgsnd(msgID, &orderMsg, strlen(orderMsg.mtext) + 1, 0);

                printf("Customer %d is ordering...\n", customer.id);

                if (waitTime > customer.patience) {
                    customer.state = LEFT;
                } else {
                    customer.state = WAITING_FOR_ORDER;
                }
                break;
            }

            case WAITING_FOR_ORDER:
                sleep(waitTime);
                printf("Customer %d is waiting for order...\n", customer.id);

                int quality = rand() % 2 + 1;
                if (quality == FEEDBACK_BAD) {
                    customer.state = COMPLAINING;
                } else {
                    printf("Customer %d is happy with good quality.\n", customer.id);
                    return 0;
                }
                break;

            case COMPLAINING:
                shared_game->num_complained_customers++;
                printf("Customer %d complained about bad quality.\n", customer.id);
                return 0;

            case LEFT:
                shared_game->num_customers_missing++;
                printf("Customer %d got frustrated and left after waiting %d seconds.\n", customer.id, waitTime);
                return 0;

            default:
                fprintf(stderr, "Customer %d entered unknown state.\n", customer.id);
                return 1;
        }
    }
}
