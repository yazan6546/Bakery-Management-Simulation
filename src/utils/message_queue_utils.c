//
// Created by yazan on 5/2/2025.
//

#include "bakery_message.h"
#include <sys/ipc.h>
#include <sys/msg.h>

// Create a message queue or get existing one
int get_message_queue() {
    key_t key = ftok(".", 'M');  // Use current directory for key generation
    int msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        perror("Failed to create/access message queue");
        return -1;
    }

    return msgid;
}