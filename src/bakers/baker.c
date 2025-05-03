

 #define _POSIX_C_SOURCE 200809L        
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/mman.h>
 #include <sys/msg.h>
 #include <string.h>
 #include <fcntl.h>
 #include <signal.h>
 #include <errno.h>
 
 #include "oven.h"
 #include "BakerTeam.h"
 #include "game.h"
 #include "bakery_utils.h"
 #include "products.h"
 #include "semaphores_utils.h"
 
 #define TEAM_COUNT      3
 #define INPUT_QUEUE_KEY 0xCAFEBABE     
 
 
 typedef struct {
     long         mtype;                
     char         item_name[MAX_NAME_LENGTH];
     ProductType  category;
     int          index;               
 } BakeryMessage;
 

 static inline int category_to_slot(ProductType cat)
 {
     if (cat == BREAD)                     return 0;   /* Bread team      */
     if (cat == CAKE || cat == SWEET)      return 1;   /* Cake/Sweet team */
     return 2;                                         /* Patisserie team */
 }
 

 static int   team_q [TEAM_COUNT] = { -1,-1,-1 };
 static int   in_q                = -1;
 static pid_t manager_pgid        = 0;  
 

 static void graceful_shutdown(int signo)
 {
     static int done = 0;           
     if (done) return;  done = 1;
 
     if (in_q != -1)             msgctl(in_q, IPC_RMID, NULL);
     for (int i = 0; i < TEAM_COUNT; ++i)
         if (team_q[i] != -1)    msgctl(team_q[i], IPC_RMID, NULL);
 
     if (manager_pgid > 0)
         kill(-manager_pgid, SIGTERM);
 
     write(STDERR_FILENO,
           "\n[manager] graceful shutdown – IPC queues removed\n", 53);
 
     if (signo != 0) _exit(0);
 }
 
 static void on_exit_wrapper(void)
 {
     graceful_shutdown(0);             
 }
 

 int main(void)
 {

     manager_pgid = getpid();
     setpgid(0, manager_pgid);
 
     /* register cleanup – both ways */
     atexit(on_exit_wrapper);
 
     struct sigaction sa = {0};
     sa.sa_handler = graceful_shutdown;
     sigemptyset(&sa.sa_mask);
     sigaction(SIGINT,  &sa, NULL);
     sigaction(SIGTERM, &sa, NULL);
 

     int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
     if (shm_fd == -1) { perror("shm_open"); exit(EXIT_FAILURE); }
 
     Game *game = mmap(NULL, sizeof(Game),
                       PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
     if (game == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }
     close(shm_fd);
 
     printf("********** Bakery Simulation (manager) **********\n");
     print_config(&game->config);

     for (int i = 0; i < TEAM_COUNT; ++i) {
         team_q[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
         if (team_q[i] == -1) { perror("msgget team"); exit(EXIT_FAILURE); }
     }
 
     /* ---------- fork baker_workers -------------------------------- */
     BakerTeam teams[TEAM_COUNT];
     distribute_bakers_locally(&game->config, teams);
 
     for (int t = 0; t < TEAM_COUNT; ++t)
         for (int b = 0; b < teams[t].number_of_bakers; ++b) {
             if (fork() == 0) {               /* child process */
                 char q_s[16], team_s[8];
                 snprintf(q_s, sizeof q_s, "%d", team_q[t]);
                 snprintf(team_s, sizeof team_s, "%d", teams[t].team_name);
                 execl("./baker_worker", "baker_worker", q_s, team_s, NULL);
                 perror("execl"); _exit(EXIT_FAILURE);
             }
         }
 
     /* ---------- public front‑door queue --------------------------- */
     in_q = msgget(INPUT_QUEUE_KEY, 0666 | IPC_CREAT);
     if (in_q == -1) { perror("msgget input"); exit(EXIT_FAILURE); }
 
     printf("Manager ready – public queue key 0x%X  (id %d)\n",
            INPUT_QUEUE_KEY, in_q);
     puts("Send BakeryMessage structs here to feed the bakers…");
 
     /* ---------- dispatcher loop ----------------------------------- */
     BakeryMessage msg;
     while (1) {
         ssize_t r = msgrcv(in_q, &msg, sizeof msg - sizeof(long), 0, 0);
         if (r == -1) {
             if (errno == EINTR) continue;          /* interrupted by signal */
             perror("manager msgrcv"); continue;
         }
 
         int slot = category_to_slot(msg.category);
         if (msgsnd(team_q[slot], &msg,
                    sizeof msg - sizeof(long), 0) == -1) {
             perror("manager msgsnd");
         } else {
             const char *team =
                 (slot == 0) ? "Bread" :
                 (slot == 1) ? "Cake/Sweet" : "Patisserie";
             printf("→ dispatched %-20s to %s queue (id %d)\n",
                    msg.item_name, team, team_q[slot]);
         }
     }
     /* never reached */
     return 0;
 }
 