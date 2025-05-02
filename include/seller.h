#ifndef SELLER_H
#define SELLER_H

typedef enum SellerState {
    IDLE,
    TAKING_ORDER,
    PROCESSING_ORDER,
    COMPLETING_ORDER
} SellerState;


typedef struct {
    int id;
    pid_t pid;
    SellerState state;
} Seller;


int send_completion_message(int msg_queue_id, pid_t customer_pid, float total_price, const char* status);
int get_message_queue();
const char* get_seller_state_string(SellerState state);
void init_seller(Seller *seller, int id);



#endif //SELLER_H