//
// Created by yazan on 5/2/2025.
//

#include "bakery_message.h"
#include <sys/ipc.h>
#include <sys/msg.h>

// Create a message queue or get existing one
int get_message_queue() {
    int msgid = msgget(CUSTOMER_SELLER_MSG_KEY, 0666 | IPC_CREAT);

    if (msgid == -1) {
        perror("Failed to create/access message queue");
        return -1;
    }

    return msgid;
}