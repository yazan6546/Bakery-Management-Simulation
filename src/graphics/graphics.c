/****************************************************************
 * graphics.c – queue‑driven rendering
 *              + diagonal “frustrated walk‑off”
 *              + frustration sound effect
 *              + baker / chef live team, state, item
 ****************************************************************/
 #include "raylib.h"
 #include "raymath.h"
 
 #include "animation.h"
 #include "game.h"
 #include "inventory.h"
 #include "products.h"
 #include "queue.h"
 #include "customer.h"
 #include "shared_mem_utils.h"
 
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 
 /* ---------- helper: readable baker team -------------------- */
 static const char* get_team_name_str(Team t){
     switch(t){
         case BREAD_BAKERS:           return "Bake Bread";
         case CAKE_AND_SWEETS_BAKERS: return "Bake Cakes & Sweets";
         case PASTRIES_BAKERS:        return "Bake Patisseries";
         default:                     return "Unknown Team";
     }
 }
 
 /* ---------- window & layout -------------------------------- */
 #ifndef ASSETS_PATH
 #define ASSETS_PATH "assets/"
 #endif
 #define WIN_W   1700
 #define WIN_H   1000
 #define DASH_W   520
 #define BAR_H    240
 #define TOP_M      0
 #define DRAW_SCALE 0.90f
 
 /* ---------- fonts ------------------------------------------ */
 #define FONT_LG 26
 #define FONT_MD 18
 #define FONT_SM 14
 #define FONT_XS 11
 
 /* ---------- queue / animation constants -------------------- */
 #define MAX_VISUALS   64
 #define MAX_LEAVERS   64
 #define QUEUE_SPACING 68
 #define WALK_SCALE    5.0f
 #define LEAVE_SPEED   180.0f   /* px per second */
 
 /* ---------- quick helpers ---------------------------------- */
 static Texture2D mustLoad(const char *p){
     Texture2D t = LoadTexture(p);
     if(!t.id){ TraceLog(LOG_FATAL,"cannot load %s",p); exit(1); }
     return t;
 }
 static Color tintForState(CustomerState s){
     switch(s){
         case WALKING:            return WHITE;
         case WAITING_IN_QUEUE:   return SKYBLUE;
         case WAITING_FOR_ORDER:  return LIME;
         case ORDERING:           return GREEN;
         case FRUSTRATED:         return RED;
         case COMPLAINING:        return ORANGE;
         case CONTAGION:          return PURPLE;
         default:                 return GRAY;
     }
 }
 
 /* ---------- sprite frame tables ---------------------------- */
 static const Rectangle walkFrames[8] = {
     {22,289,20,48},{44,289,17,48},{63,289,12,48},{77,289,14,48},
     {97,289,20,49},{120,289,15,48},{140,289,13,48},{158,289,14,48}};
 static const Rectangle frustFrames[4] = {
     {0,0,20,48},{20,0,20,48},{40,0,20,48},{60,0,20,48}};
 static const int idleFrameIdx = 0;
 
 /* ---------- cached queue visuals --------------------------- */
 typedef struct { Animation *anim; CustomerState last; } Visual;
 static Visual visuals[MAX_VISUALS] = {0};
 
 static void ensureAnim(Texture2D sh, Visual *v, CustomerState st){
     const Rectangle *set = walkFrames; int len=8; float fps=8.f;
     if(st==FRUSTRATED){ set=frustFrames; len=4; fps=12.f; }
     if(!v->anim || v->last!=st){
         FreeAnimation(&v->anim);
         v->anim = CreateAnimation(sh,(Rectangle*)set,len,fps);
         v->last = st;
     }
 }
 
 /* ---------- leaver tracking -------------------------------- */
 typedef struct{
     const Customer *ref;
     Animation *anim;
     float x,y,dx,dy;
     bool active;
 } Leaver;
 static Leaver leavers[MAX_LEAVERS] = {0};
 
 /* ---------- frustration sound ------------------------------ */
 static Sound frustrSound = {0};
 static void playFrust(){ if(frustrSound.frameCount) PlaySound(frustrSound); }
 
 /* =========================================================== */
 int main(void)
 {
     /* ---- shared memory ------------------------------------ */
     Game *g=NULL;          setup_shared_memory(&g);
     queue_shm *custQ=NULL; setup_queue_shared_memory(&custQ,
                                        g->config.MAX_CUSTOMERS);
 
     /* ---- assets ------------------------------------------- */
     InitWindow(WIN_W,WIN_H,"Bakery GUI");
     InitAudioDevice();
 
     Texture2D bg    = mustLoad(ASSETS_PATH "backgroundnew.png");
     Texture2D sheet = mustLoad(ASSETS_PATH "characters/customer/SpriteSheet.png");
     Texture2D ovenT = mustLoad(ASSETS_PATH "oven.png");
     Texture2D chefT = mustLoad(ASSETS_PATH "characters/chef/ChefSheet.png");
     Texture2D sellT = mustLoad(ASSETS_PATH "seller.png");
     Texture2D bakeT = mustLoad(ASSETS_PATH "baker.png");
     frustrSound     = LoadSound(ASSETS_PATH "frustrated.wav");
 
     /* sprite sizes & geometry ------------------------------- */
     const float SELL_SCALE=0.12f;
     const float SELL_W=sellT.width *SELL_SCALE;
     const float SELL_H=sellT.height*SELL_SCALE;
 
     const Rectangle chefFrame={7,2,32,52};
     const float CHEF_SCALE=2.0f, BAK_R=30, CHEF_S=60;
 
     const int   BG_W=WIN_W-DASH_W, BG_H=WIN_H-BAR_H;
     const float SCALE   = (BG_H/(float)bg.height)*DRAW_SCALE;
     const float WORLD_W = bg.width*SCALE;
     const float drawH   = bg.height*SCALE;
     const float groundY = TOP_M + drawH - walkFrames[0].height*WALK_SCALE/2;
     const float sellerY = TOP_M + drawH - 200;
 
     const float EXIT_X = WORLD_W + 200;
     const float EXIT_Y = groundY + 40;
 
     Camera2D cam={0};
     float vScroll=0, staffScroll=0;
 
     /* ---------------- main loop ---------------------------- */
     while(!WindowShouldClose())
     {
         float dt=GetFrameTime();
 
         /* live counts & pointers ----------------------------- */
         int nBakers = g->config.NUM_BAKERS;
         int nChefs  = g->config.NUM_CHEFS;
         int nSell   = g->config.NUM_SELLERS;
 
         Baker *bakers = g->info.bakers;
         Chef  *chefs  = g->info.chefs;
 
         /* camera -------------------------------------------- */
         if(IsKeyDown(KEY_RIGHT)) cam.target.x += 8;
         if(IsKeyDown(KEY_LEFT )) cam.target.x -= 8;
         cam.target.x = Clamp(cam.target.x,0,WORLD_W-BG_W);
 
         if(IsKeyDown(KEY_DOWN)) vScroll += 8;
         if(IsKeyDown(KEY_UP  )) vScroll -= 8;
         if(vScroll<0) vScroll=0;
 
         /* staff bar scroll ---------------------------------- */
         if(GetMouseY()>WIN_H-BAR_H) staffScroll -= GetMouseWheelMove()*40;
         if(IsKeyDown(KEY_LEFT_BRACKET ))  staffScroll -= 8;
         if(IsKeyDown(KEY_RIGHT_BRACKET))  staffScroll += 8;
         float bakerW  = 120 + nBakers*(BAK_R*2+140);
         float chefW   = 120 + nChefs *(CHEF_S+180);
         float contentW= bakerW>chefW ? bakerW : chefW;
         staffScroll   = Clamp(staffScroll,0,contentW>WIN_W?contentW-WIN_W:0);
 
         /* ================= DRAW ============================ */
         BeginDrawing();
         ClearBackground(RAYWHITE);
 
         /* ---- world clip ----------------------------------- */
         BeginScissorMode(0,0,BG_W,BG_H);
         {
             Rectangle src={0,0,bg.width,bg.height};
             Rectangle dst={-cam.target.x,TOP_M-vScroll,bg.width*SCALE,drawH};
             DrawTexturePro(bg,src,dst,(Vector2){0,0},0,WHITE);
 
             /* sellers */
             float sp=WORLD_W/(nSell+1.f);
             for(int i=0;i<nSell;i++){
                 float xs=-cam.target.x+sp*(i+1)-SELL_W/2;
                 Rectangle d={xs,sellerY-SELL_H-vScroll-40,SELL_W,SELL_H};
                 DrawTexturePro(sellT,(Rectangle){0,0,sellT.width,sellT.height},
                                d,(Vector2){0,0},0,WHITE);
             }
 
             /* queue customers */
             int qN = custQ->count>MAX_VISUALS?MAX_VISUALS:custQ->count;
             for(int i=0;i<qN;i++){
                 int qi=(custQ->head+i)%custQ->capacity;
                 Customer *c=&((Customer*)custQ->elements)[qi];
 
                 /* frustrated → leaver ------------------------- */
                 if(c->state==FRUSTRATED){
                     int slot=-1;
                     for(int l=0;l<MAX_LEAVERS;l++){
                         if(leavers[l].active && leavers[l].ref==c){ slot=-2; break;}
                         if(!leavers[l].active && slot==-1) slot=l;
                     }
                     if(slot>=0){
                         float sx=sp-SELL_W/2-60-i*QUEUE_SPACING;
                         leavers[slot]=(Leaver){
                             .ref=c,.x=sx,.y=groundY-16,.active=true,
                             .anim=CreateAnimation(sheet,(Rectangle*)frustFrames,4,12)};
                         Vector2 dir=Vector2Normalize((Vector2){EXIT_X-sx,EXIT_Y-(groundY-16)});
                         leavers[slot].dx=dir.x; leavers[slot].dy=dir.y;
                         playFrust();
                     }
                     continue;
                 }
 
                 Visual *v=&visuals[i]; ensureAnim(sheet,v,c->state);
                 UpdateAnimation(v->anim,dt);
                 if(c->state!=WALKING) v->anim->currentFrame=idleFrameIdx;
 
                 float px=sp-SELL_W/2-60-i*QUEUE_SPACING;
                 Rectangle fr=v->anim->frames[v->anim->currentFrame];
                 float w=fr.width*WALK_SCALE, h=fr.height*WALK_SCALE;
                 Rectangle dr={px-cam.target.x,groundY-vScroll,w,h};
                 DrawAnimationPro(v->anim,dr,(Vector2){w/2,h/2},0,
                                  tintForState(c->state));
             }
 
             /* leavers update & draw -------------------------- */
             for(int l=0;l<MAX_LEAVERS;l++) if(leavers[l].active){
                 Animation *a=leavers[l].anim;
                 UpdateAnimation(a,dt);
                 leavers[l].x+=leavers[l].dx*LEAVE_SPEED*dt;
                 leavers[l].y+=leavers[l].dy*LEAVE_SPEED*dt;
 
                 Rectangle fr=a->frames[a->currentFrame];
                 float w=fr.width*WALK_SCALE, h=fr.height*WALK_SCALE;
                 Rectangle dr={leavers[l].x-cam.target.x,
                               leavers[l].y-vScroll,w,h};
                 DrawAnimationPro(a,dr,(Vector2){w/2,h/2},0,RED);
 
                 if(leavers[l].x>EXIT_X){
                     leavers[l].active=false;
                     FreeAnimation(&leavers[l].anim);
                 }
             }
         }
         EndScissorMode();
 
         /* ---- right dashboard (unchanged) ------------------- */
         int rx=BG_W;
         DrawRectangle(rx,0,DASH_W,WIN_H,(Color){245,245,245,255});
         DrawText("Game Dashboard",rx+10,10,FONT_LG,DARKGRAY);
         DrawText(TextFormat("Time  : %d s",g->elapsed_time),rx+10,50,FONT_MD,BLACK);
         DrawText(TextFormat("Profit: %.2f",g->daily_profit),rx+10,70,FONT_MD,BLACK);
 
         int y=100;
         DrawText("Customers:",rx+10,y,FONT_MD,DARKGRAY); y+=20;
         DrawText(TextFormat("Served        : %d",g->num_customers_served),rx+10,y,FONT_XS,BLACK); y+=14;
         DrawText(TextFormat("Frustrated    : %d",g->num_frustrated_customers),rx+10,y,FONT_XS,BLACK); y+=14;
         DrawText(TextFormat("Complained    : %d",g->num_complained_customers),rx+10,y,FONT_XS,BLACK); y+=14;
         DrawText(TextFormat("Missing Item  : %d",g->num_customers_missing),rx+10,y,FONT_XS,BLACK); y+=14;
         DrawText(TextFormat("Cascade Events: %d",g->num_customers_cascade),rx+10,y,FONT_XS,BLACK); y+=20;
 
         /* ---- ready products / ingredients / ovens ---------- */
         int colW=(DASH_W-20)/2, readyX=rx+10, ingrX=rx+10+colW;
         int yR=y, yI=y;
         DrawText("Ready products:",readyX,yR,FONT_MD,DARKGRAY); yR+=20;
         DrawText("Ingredients:",   ingrX ,yI,FONT_MD,DARKGRAY);  yI+=20;
         for(int cat=0;cat<NUM_PRODUCTS;cat++){
             const ProductCategory *pc=&g->productCatalog.categories[cat];
             if(pc->product_count==0) continue;
             DrawText((const char*[]){"Bread","Cake","Sandwich","Sweet",
                                       "SweetPat.","SavoryPat."}[cat],
                      readyX,yR,FONT_SM,MAROON); yR+=16;
             for(int p=0;p<pc->product_count;p++){
                 int q=g->ready_products.categories[cat].quantities[p];
                 DrawText(TextFormat("\u2022 %s: %d",pc->products[p].name,q),
                          readyX+12,yR,FONT_XS,BLACK); yR+=14;
                 if(yR>WIN_H-BAR_H-160){
                     DrawText("...",readyX+12,yR,FONT_XS,BLACK); goto SKIP_ING;
                 }
             }
         }
 SKIP_ING:
         for(int ing=0;ing<NUM_INGREDIENTS;ing++){
             float q=g->inventory.quantities[ing];
             DrawText(TextFormat("%s: %.1f",get_ingredient_name(ing),q),
                      ingrX,yI,FONT_XS,BLACK); yI+=14;
             if(yI>WIN_H-BAR_H-160){
                 DrawText("...",ingrX,yI,FONT_XS,BLACK); break;
             }
         }
 
         /* ovens --------------------------------------------- */
         float ovensY=yI+20;
         if(ovensY+ovenT.height>WIN_H-BAR_H-10)
             ovensY=WIN_H-BAR_H-ovenT.height-10;
         for(int o=0;o<g->config.NUM_OVENS;o++){
             int x=rx+10+o*(ovenT.width+40);
             DrawTexture(ovenT,x,ovensY,WHITE);
             Oven ov=g->ovens[o];
             DrawText(ov.is_busy?"Preparing":"Idle",
                      x,ovensY+ovenT.height+4,FONT_XS,ov.is_busy?RED:DARKGREEN);
             DrawText(ov.item_name,x,ovensY+ovenT.height+18,FONT_XS,BLACK);
             if(ov.is_busy)
                 DrawText(TextFormat("%ds left",ov.time_left),
                          x,ovensY+ovenT.height+32,FONT_XS,MAROON);
         }
 
         /* ---- staff bar ------------------------------------ */
         int by=WIN_H-BAR_H;
         DrawRectangle(0,by,WIN_W,BAR_H,(Color){235,235,235,255});
 
         /* bakers -------------------------------------------- */
         DrawText("Bakers:",10,by+10,FONT_MD,DARKGRAY);
         float bx=120-staffScroll;
         for(int i=0;i<nBakers;i++){
             Baker *bk=&bakers[i];
             const char *teamStr = get_team_name_str(bk->team_name);
             const char *stateStr= bk->state==0? "Idle":"Busy";
 
             /* show item only when busy */
             char bItem[64];
             if(bk->state==0) bItem[0]='\0';
             else { strncpy(bItem,bk->Item,sizeof(bItem)-1); bItem[sizeof(bItem)-1]='\0'; }
             const char *itemStr = strlen(bItem)? bItem:"";
 
             Rectangle dst={bx-32,by+15,64,96};
             DrawTexturePro(bakeT,(Rectangle){0,0,bakeT.width,bakeT.height},
                            dst,(Vector2){0,0},0,WHITE);
             DrawText(TextFormat("State %s",stateStr), bx+BAK_R+10,by+20,FONT_XS,BLACK);
             DrawText(teamStr,                           bx+BAK_R+10,by+35,FONT_XS,BLACK);
             if(strlen(itemStr))
                 DrawText(TextFormat("Item %s",itemStr),bx+BAK_R+10,by+50,FONT_XS,BLACK);
             bx += BAK_R*2 + 140;
         }
 
         /* chefs --------------------------------------------- */
         DrawText("Chefs:",10,by+120,FONT_MD,DARKGRAY);
         float cx=120-staffScroll;
         for(int i=0;i<nChefs;i++){
             Chef *ch=&chefs[i];
             const char *stateStr = ch->is_active? "Busy":"Idle";
 
             char cItem[64];
             if(!ch->is_active) cItem[0]='\0';
             else { strncpy(cItem,ch->Item,sizeof(cItem)-1); cItem[sizeof(cItem)-1]='\0'; }
             const char *itemStr = strlen(cItem)? cItem:"";
 
             float top=by+145;
             Rectangle dst={cx-(chefFrame.width*CHEF_SCALE)/2,
                            top,
                            chefFrame.width*CHEF_SCALE,
                            chefFrame.height*CHEF_SCALE};
             DrawTexturePro(chefT,chefFrame,dst,(Vector2){0,0},0,WHITE);
             DrawText(TextFormat("State %s",stateStr), cx+CHEF_S/2+10,top-10,FONT_XS,BLACK);
             DrawText(TextFormat("Team %d",ch->team),  cx+CHEF_S/2+10,top+5 ,FONT_XS,BLACK);
             if(strlen(itemStr))
                 DrawText(TextFormat("Item %s",itemStr),cx+CHEF_S/2+10,top+20,FONT_XS,BLACK);
             cx += CHEF_S + 180;
         }
 
         /* scroll bar indicator ------------------------------ */
         if(contentW>WIN_W){
             float bw=(WIN_W/contentW)*WIN_W;
             float bxBar=(staffScroll/(contentW-WIN_W))*(WIN_W-bw);
             DrawRectangle(bxBar,by-8,bw,4,(Color){120,120,120,200});
         }
 
         EndDrawing();
     }
 
     /* cleanup ----------------------------------------------- */
     for(int i=0;i<MAX_VISUALS;i++) FreeAnimation(&visuals[i].anim);
     for(int l=0;l<MAX_LEAVERS;l++) if(leavers[l].active)
         FreeAnimation(&leavers[l].anim);
     UnloadTexture(bakeT);UnloadTexture(chefT);UnloadTexture(sellT);
     UnloadTexture(ovenT);UnloadTexture(sheet);UnloadTexture(bg);
     UnloadSound(frustrSound); CloseAudioDevice();
 
     munmap(custQ,queueShmSize(sizeof(Customer),g->config.MAX_CUSTOMERS));
     cleanup_shared_memory(g);
     CloseWindow();
     return 0;
 }
 