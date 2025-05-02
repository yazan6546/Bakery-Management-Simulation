//
// Created by yazan on 5/3/2025.
//

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

sig_atomic_t count=0;
void handle_alarm(int signum) {
    count++;
    printf("Alarm triggered %d times\n", count);
    alarm(1);
}
int main() {
    signal(SIGALRM, handle_alarm);
    alarm(1);

    int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Failed to create message queue");
        return 1;
    }

    if (msgrcv(msgid, NULL, sizeof(int), 0, 0) == -1) {
        perror("Failed to receive message");
    }
}