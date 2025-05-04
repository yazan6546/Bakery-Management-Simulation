/****************************************************************
 * baker_worker.c  – baker process
 * (calls to oven_tick removed; ovens are now decremented by main)
 ****************************************************************/
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/msg.h>
 #include <string.h>
 #include <sys/mman.h>
 #include <semaphore.h>
 #include <errno.h>
 
 #include "oven.h"
 #include "inventory.h"
 #include "game.h"
 #include "team.h"
 #include "bakery_utils.h"
 #include "products.h"
 #include "semaphores_utils.h"
 #include "bakery_message.h"
 
 #define MAX_NAME MAX_NAME_LENGTH
 
 static inline Team category_to_team(ProductType c)
 {
     switch (c) {
         case BREAD:  return BREAD_BAKERS;
         case CAKE:
         case SWEET:  return CAKE_AND_SWEETS_BAKERS;
         default:     return PASTRIES_BAKERS;
     }
 }
 
 static Game *game   = NULL;
 
 int main(int argc, char *argv[])
 {
     if (argc != 4) {
         fprintf(stderr,"Usage: %s <mqid> <team_enum> <baker_id>\n",argv[0]);
         return EXIT_FAILURE;
     }
 
     /* ---- parse args -------------------------------------- */
     int  mqid    = atoi(argv[1]);
     Team my_team = (Team)atoi(argv[2]);
     int  id      = atoi(argv[3]);
 
     printf("Baker %d started in team %s\n", id, get_team_name_str(my_team));
 
     /* ---- shared memory ----------------------------------- */
     int shm_fd = shm_open("/game_shared_mem", O_RDWR, 0666);
     if (shm_fd == -1){ perror("shm_open"); exit(EXIT_FAILURE); }
     game = mmap(NULL,sizeof(Game),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
     if (game == MAP_FAILED){ perror("mmap"); exit(EXIT_FAILURE); }
     close(shm_fd);
 
     /* ---- semaphores -------------------------------------- */
     sem_t *ready_sem = setup_ready_products_semaphore();
     setup_oven_semaphores(game->config.NUM_OVENS);
 
     /* ---- initial state ----------------------------------- */
     game->info.bakers[id].state = BAKER_IDLE;
     ChefMessage cur_msg = {0};
     int oven_idx        = -1;
 
     /* ============ event loop ============================== */
     for(;;){
         /* ---------- currently baking? ---------------------- */
         if (game->info.bakers[id].state == BAKER_BUSY && oven_idx != -1) {
 
             /* main process now decrements ovens every second.
                When an oven finishes, is_busy becomes 0.
             */
             if (!game->ovens[oven_idx].is_busy) {
                 /* move finished product to ready‑products ----- */
                 ProductType tp = get_product_type_for_team(cur_msg.source_team);
                 add_ready_product(&game->ready_products,
                                   tp,
                                   cur_msg.product_index,
                                   1,
                                   ready_sem);
                 printf("[Baker %s] Finished %s in oven %d\n",
                        get_team_name_str(my_team),
                        cur_msg.product_name, oven_idx);
 
                 game->info.bakers[id].state = BAKER_IDLE;
                 oven_idx = -1;
             } else {
                 /* still baking – just sleep a bit              */
                 sleep(1);
                 continue;
             }
         }
 
         /* ---------- idle: wait for new job ----------------- */
         if (game->info.bakers[id].state == BAKER_IDLE) {
             ChefMessage msg;
             ssize_t r = msgrcv(mqid,&msg,sizeof(ChefMessage)-sizeof(long),
                                game->info.bakers[id].team_name,0);
             if (r < 0) {
                 if (errno==EIDRM || errno==EINVAL) _exit(0);
                 if (errno==EINTR) continue;
                 perror("[baker] msgrcv"); continue;
             }
 
             if (get_baker_team_from_chef_team(msg.source_team) != my_team){
                 /* not for me – push back and retry */
                 msgsnd(mqid,&msg,sizeof(ChefMessage)-sizeof(long),0);
                 usleep(10000);
                 continue;
             }
 
             cur_msg = msg;
 
             int prep = game->config.MIN_BAKE_TIME +
                        rand()%(game->config.MAX_BAKE_TIME
                               -game->config.MIN_BAKE_TIME+1);
 
             strncpy(game->info.bakers[id].Item,
                     cur_msg.product_name, MAX_NAME_LENGTH-1);
             game->info.bakers[id].Item[MAX_NAME_LENGTH-1]='\0';
 
             printf("[Baker %s] Preparing %s (%d s)\n",
                    get_team_name_str(my_team), cur_msg.product_name, prep);
             sleep(prep);
             printf("[Baker %s] %s finished preparation\n",
                    get_team_name_str(my_team), cur_msg.product_name);
 
             /* try to find free oven -------------------------- */
             while(1){
                 for(int i=0;i<game->config.NUM_OVENS;++i){
                     int bake_time = game->config.MIN_OVEN_TIME +
                                     rand()%(game->config.MAX_OVEN_TIME
                                            -game->config.MIN_OVEN_TIME+1);
                     if (put_item_in_oven(&game->ovens[i],
                                          cur_msg.product_name,
                                          get_team_name_str(my_team),
                                          bake_time)){
                         oven_idx = i;
                         game->info.bakers[id].state = BAKER_BUSY;
                         printf("[Baker %s] Placed %s in oven %d for %d s\n",
                                get_team_name_str(my_team),
                                cur_msg.product_name,i,bake_time);
                         goto next_iteration;
                     }
                 }
                 /* no oven free yet — wait until main ticks one */
                 printf("[Baker %s] No oven free – waiting\n",
                        get_team_name_str(my_team));
                 sleep(1);
             }
         }
 next_iteration:
         continue;
     }
     return 0;
 }
 