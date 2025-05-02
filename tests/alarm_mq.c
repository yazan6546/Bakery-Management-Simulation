//
// Created by yazan on 5/3/2025.
//

#include "signal.h"
sig_atomic_t count=0;
void handle_alarm(int signum) {
    count++;
}
int main() {
    signal(SIGALRM, handle_alarm);
    alarm(1);
    while (count < 5) {
        pause();
    }
    printf("Alarm triggered 5 times\n");
    return 0;
}