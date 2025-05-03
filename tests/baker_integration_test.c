/*  tests/producer.c
 *
 *  Sends random BakeryMessage jobs to the public input queue
 *  (key 0xCAFEBABE) used by baker.c.
 *
 *  Build:
 *      add_executable(producer tests/producer.c)
 *      target_link_libraries(producer PRIVATE rt)
 */

 #define _POSIX_C_SOURCE 200809L
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/msg.h>
 #include <time.h>
 
 #include "products.h"          /* ProductType enum */
 
 #define INPUT_QUEUE_KEY 0xCAFEBABE
 #define MAX_NAME         MAX_NAME_LENGTH
 
 typedef struct {
     long         mtype;        /* 1 bread, 2 cake/sweet, 3 patis */
     char         item_name[MAX_NAME];
     ProductType  category;
     int          index;        /* index inside its JSON list     */
 } BakeryMessage;
 
 /* ------------------------------------------------------------ */
 static void send_job(int qid, long mtype,
                      ProductType cat,
                      const char *name,
                      int index)
 {
     BakeryMessage m;
     m.mtype    = mtype;
     m.category = cat;
     m.index    = index;
     strncpy(m.item_name, name, MAX_NAME - 1);
     m.item_name[MAX_NAME - 1] = '\0';
 
     if (msgsnd(qid, &m, sizeof m - sizeof(long), 0) == -1)
         perror("msgsnd");
     else
         printf("» produced %-17s (cat=%d idx=%d)\n",
                name, cat, index);
 }
 
 /* ------------------------------------------------------------ */
 int main(void)
 {
     int in_q = msgget(INPUT_QUEUE_KEY, 0666 | IPC_CREAT);
     if (in_q == -1) { perror("msgget"); return 1; }
 
     const char *bread[] = { "White Bread", "Baguette", "Wheat Bread" };
     const char *cake [] = { "Chocolate Cake", "Vanilla Cake" };
     const char *patis[] = { "Fruit Tart", "Eclair" };
 
     srand((unsigned)time(NULL));
     puts("producer: sending jobs every 1‑2 s – Ctrl‑C to stop");
 
     for (;;)
     {
         int sel = rand() % 3;
 
         if (sel == 0) {
             int idx = rand() % 3;
             send_job(in_q, 1, BREAD, bread[idx], idx);
         } else if (sel == 1) {
             int idx = rand() % 2;
             /* cake/sweet team ⇒ CAKE category */
             send_job(in_q, 2, CAKE, cake[idx], idx);
         } else {
             int idx = rand() % 2;
             send_job(in_q, 3, SWEET_PATISSERIES, patis[idx], idx);
         }
 
         sleep(1 + rand() % 2);
     }
 }
 