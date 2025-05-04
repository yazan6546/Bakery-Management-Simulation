/****************************************************************
 *  graphics.c  –  queue‑driven customer rendering
 ****************************************************************/
 #include "raylib.h"
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
 #include "raymath.h"     // adds Clamp(), Vector2Clamp(), etc.

 
 /* ---------------- paths & dimensions ------------------------- */
 #ifndef ASSETS_PATH
 #define ASSETS_PATH "assets/"
 #endif
 #define WIN_W  1700
 #define WIN_H  1000
 #define DASH_W  520
 #define BAR_H   240
 #define TOP_M     0
 #define DRAW_SCALE 0.90f
 
 /* ---------------- fonts -------------------------------------- */
 #define FONT_LG 26
 #define FONT_MD 18
 #define FONT_SM 14
 #define FONT_XS 11
 
 /* ---------------- customer visuals --------------------------- */
 #define MAX_VISUALS   64
 #define QUEUE_SPACING 68
 #define WALK_SCALE    5.0f       /* magnification for customer sheet */
 
 /* ---------------- quick loader ------------------------------- */
 static Texture2D mustLoad(const char* p){
     Texture2D t = LoadTexture(p);
     if(!t.id){ TraceLog(LOG_FATAL,"cannot load %s",p); exit(1); }
     return t;
 }
 
 /* ---------------- per‑state helpers -------------------------- */
 static Color tintForState(CustomerState st){
     switch(st){
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
 static const char* nameForState(CustomerState st){
     switch(st){
         case WALKING:            return "Walking";
         case WAITING_IN_QUEUE:   return "Queue";
         case WAITING_FOR_ORDER:  return "Waiting";
         case ORDERING:           return "Ordering";
         case FRUSTRATED:         return "Frustrated";
         case COMPLAINING:        return "Complain";
         case CONTAGION:          return "Contagion";
         default:                 return "?";
     }
 }
 
 /* ---------------- sprite frames ------------------------------ */
 static const Rectangle walkFrames[8] = {
     {22,289,20,48},{44,289,17,48},{63,289,12,48},{77,289,14,48},
     {97,289,20,49},{120,289,15,48},{140,289,13,48},{158,289,14,48}
 };
 static const Rectangle frustratedFrames[4] = {
     {0,0,20,48},{20,0,20,48},{40,0,20,48},{60,0,20,48}
 };
 static const int idleFrameIdx = 0;
 
 /* ---------------- visual slot --------------------------------*/
 typedef struct{
     Animation* anim;
     CustomerState lastState;
 } Visual;
 static Visual visuals[MAX_VISUALS] = {0};
 
 static void ensureAnim(Texture2D sheet, Visual* v, CustomerState st){
     const Rectangle* set = walkFrames;
     int   len = 8;
     float fps = 8.f;
     if(st==FRUSTRATED){ set=frustratedFrames; len=4; fps=12.f; }
 
     if(!v->anim || v->lastState!=st){
         FreeAnimation(&v->anim);
         v->anim = CreateAnimation(sheet,(Rectangle*)set,len,fps);
         v->lastState = st;
     }
 }
 
 /* ============================================================= */
 int main(void)
 {
     /* ---------- shared memory ---------------------------------- */
     Game *g=NULL; setup_shared_memory(&g);
     queue_shm *custQueue=NULL;
     setup_queue_shared_memory(&custQueue, g->config.MAX_CUSTOMERS);
 
     /* ---------- assets ---------------------------------------- */
     InitWindow(WIN_W,WIN_H,"Bakery GUI");
     Texture2D bg        = mustLoad(ASSETS_PATH "backgroundnew.png");
     Texture2D sheet     = mustLoad(ASSETS_PATH "characters/customer/SpriteSheet.png");
     Texture2D ovenTex   = mustLoad(ASSETS_PATH "oven.png");
     Texture2D chefTex   = mustLoad(ASSETS_PATH "characters/chef/ChefSheet.png");
     Texture2D sellerTex = mustLoad(ASSETS_PATH "seller.png");
     Texture2D bakerTex  = mustLoad(ASSETS_PATH "baker.png");
 
     /* seller metrics */
     const float SELL_SCALE = 0.12f;
     const float SELL_W     = sellerTex.width  * SELL_SCALE;
     const float SELL_H     = sellerTex.height * SELL_SCALE;
 
     /* chef metrics */
     const Rectangle chefFrame = {7,2,32,52};
     const float CHEF_SCALE = 2.0f;
     const float BAK_R      = 30;
     const float CHEF_S     = 60;
 
     /* world sizing */
     const int   BG_W = WIN_W - DASH_W;
     const int   BG_H = WIN_H - BAR_H;
     const float BASE_SCALE = (float)BG_H / bg.height;
     const float SCALE      = BASE_SCALE * DRAW_SCALE;
     const float WORLD_W    = bg.width * SCALE;
     const float drawH      = bg.height * SCALE;
     const float sellerY    = TOP_M + drawH - 200;
     const float groundY    = TOP_M + drawH - walkFrames[0].height*WALK_SCALE/2;
 
     Camera2D cam = {0};
     float verticalScroll = 0, staffScroll = 0;
 
     /* ---------------- loop ------------------------------------ */
     while(!WindowShouldClose())
     {
         /* ----- pull counters ----------------------------------- */
         int   nSell = g->config.NUM_SELLERS,
               nBake = g->config.NUM_BAKERS,
               nChef = g->config.NUM_CHEFS,
               nOven = g->config.NUM_OVENS;
         int   elapsed = g->elapsed_time;
         float profit  = g->daily_profit;
 
         int served  = g->num_customers_served;
         int frustr  = g->num_frustrated_customers;
         int compl   = g->num_complained_customers;
         int missing = g->num_customers_missing;
         int cascad  = g->num_customers_cascade;
 
         /* ----- camera & scroll -------------------------------- */
         if(IsKeyDown(KEY_RIGHT)) cam.target.x += 8;
         if(IsKeyDown(KEY_LEFT )) cam.target.x -= 8;
         cam.target.x = Clamp(cam.target.x,0,WORLD_W-BG_W);
 
         if(IsKeyDown(KEY_DOWN)) verticalScroll += 8;
         if(IsKeyDown(KEY_UP  )) verticalScroll -= 8;
         if(verticalScroll<0) verticalScroll=0;
 
         /* staff bar scroll */
         if(GetMouseY()>WIN_H-BAR_H)
             staffScroll -= GetMouseWheelMove()*40;
         if(IsKeyDown(KEY_LEFT_BRACKET )) staffScroll -= 8;
         if(IsKeyDown(KEY_RIGHT_BRACKET)) staffScroll += 8;
         float bakerW   = 120 + nBake*(BAK_R*2+140);
         float chefW    = 120 + nChef*(CHEF_S+180);
         float contentW = bakerW>chefW ? bakerW : chefW;
         float maxScroll= contentW>WIN_W ? contentW-WIN_W : 0;
         staffScroll = Clamp(staffScroll,0,maxScroll);
 
         /* ================= DRAW =============================== */
         BeginDrawing();
         ClearBackground(RAYWHITE);
 
         /* ---- main world clip -------------------------------- */
         BeginScissorMode(0,0,BG_W,BG_H);
         {
             Rectangle src={0,0,bg.width,bg.height};
             Rectangle dst={-cam.target.x,TOP_M-verticalScroll,
                            bg.width*SCALE,drawH};
             DrawTexturePro(bg,src,dst,(Vector2){0,0},0,WHITE);
 
             /* sellers */
             float sp = WORLD_W/(nSell+1.f);
             for(int i=0;i<nSell;i++){
                 float x=-cam.target.x+sp*(i+1)-SELL_W/2;
                 Rectangle sSrc={0,0,(float)sellerTex.width,(float)sellerTex.height};
                 Rectangle sDst={x,sellerY-SELL_H-verticalScroll-40,SELL_W,SELL_H};
                 DrawTexturePro(sellerTex,sSrc,sDst,(Vector2){0,0},0,WHITE);
             }
 
             /* customers from queue */
             int visCount = custQueue->count > MAX_VISUALS ? MAX_VISUALS
                                                           : (int)custQueue->count;
             for(int i=0;i<visCount;i++){
                 int idx = (custQueue->head + i) % custQueue->capacity;
                 Customer *cust = &((Customer*)custQueue->elements)[idx];
 
                 Visual *vis = &visuals[i];
                 ensureAnim(sheet,vis,cust->state);
                 UpdateAnimation(vis->anim, GetFrameTime());
                 if(cust->state != WALKING) vis->anim->currentFrame = idleFrameIdx;
 
                 float posX = sp - SELL_W/2 - 60 - i*QUEUE_SPACING;
                 Rectangle fr = vis->anim->frames[vis->anim->currentFrame];
                 float w = fr.width*WALK_SCALE, h = fr.height*WALK_SCALE;
                 Rectangle dr={posX - cam.target.x, groundY-verticalScroll, w,h};
                 Vector2 origin={w/2,h/2};
 
                 DrawAnimationPro(vis->anim,dr,origin,0,tintForState(cust->state));
                 DrawText(nameForState(cust->state),
                          (int)(dr.x-20),(int)(dr.y-h/2-14),FONT_XS,BLACK);
             }
         }
         EndScissorMode();
 
         /* ---- right dashboard -------------------------------- */
         int rx = BG_W;
         DrawRectangle(rx,0,DASH_W,WIN_H,(Color){245,245,245,255});
         DrawText("Game Dashboard", rx+10,10,FONT_LG,DARKGRAY);
         DrawText(TextFormat("Time  : %d s",elapsed), rx+10,50,FONT_MD,BLACK);
         DrawText(TextFormat("Profit: %.2f",profit),  rx+10,70,FONT_MD,BLACK);
 
         int yS=100;
         DrawText("Customers:", rx+10,yS,FONT_MD,DARKGRAY); yS+=20;
         DrawText(TextFormat("Served        : %d",served ), rx+10,yS,FONT_XS,BLACK); yS+=14;
         DrawText(TextFormat("Frustrated    : %d",frustr ), rx+10,yS,FONT_XS,BLACK); yS+=14;
         DrawText(TextFormat("Complained    : %d",compl  ), rx+10,yS,FONT_XS,BLACK); yS+=14;
         DrawText(TextFormat("Missing Item  : %d",missing), rx+10,yS,FONT_XS,BLACK); yS+=14;
         DrawText(TextFormat("Cascade Events: %d",cascad ), rx+10,yS,FONT_XS,BLACK); yS+=20;
 
         /* ready‑products & ingredients ----------------------- */
         int colW=(DASH_W-20)/2, readyX=rx+10, ingrX=rx+10+colW;
         int yR=yS, yI=yS;
         DrawText("Ready products:", readyX,yR,FONT_MD,DARKGRAY); yR+=20;
         DrawText("Ingredients:",    ingrX ,yI,FONT_MD,DARKGRAY);  yI+=20;
 
         for(int pt=0;pt<NUM_PRODUCTS;pt++){
             const ProductCategory *pc=&g->productCatalog.categories[pt];
             if(pc->product_count==0) continue;
             DrawText((const char*[]){"Bread","Cake","Sandwich","Sweet",
                                       "SweetPat.","SavoryPat."}[pt],
                      readyX,yR,FONT_SM,MAROON); yR+=16;
             for(int i=0;i<pc->product_count;i++){
                 int q=g->ready_products.categories[pt].quantities[i];
                 DrawText(TextFormat("\u2022 %s: %d",pc->products[i].name,q),
                          readyX+12,yR,FONT_XS,BLACK); yR+=14;
                 if(yR>WIN_H-BAR_H-160){
                     DrawText("...",readyX+12,yR,FONT_XS,BLACK); goto ING;
                 }
             }
         }
 ING:
         for(int ing=0; ing<NUM_INGREDIENTS; ing++){
             float q=g->inventory.quantities[ing];
             DrawText(TextFormat("%s: %.1f", get_ingredient_name(ing), q),
                      ingrX,yI,FONT_XS,BLACK); yI+=14;
             if(yI>WIN_H-BAR_H-160){
                 DrawText("...",ingrX,yI,FONT_XS,BLACK); break;
             }
         }
 
         /* ovens --------------------------------------------------- */
         float ovensY = yI + 20;
         if(ovensY + ovenTex.height > WIN_H - BAR_H - 10)
             ovensY = WIN_H - BAR_H - ovenTex.height - 10;
         int ovensX = rx + 10;
         for(int o=0;o<nOven;o++){
             int x = ovensX + o*(ovenTex.width + 40);
             DrawTexture(ovenTex,x,ovensY,WHITE);
             Oven oven = g->ovens[o];
             DrawText(oven.is_busy ? "Preparing" : "Idle",
                      x, ovensY + ovenTex.height + 4, FONT_XS,
                      oven.is_busy ? RED : DARKGREEN);
             DrawText(oven.item_name,
                      x, ovensY + ovenTex.height + 18, FONT_XS, BLACK);
             if(oven.is_busy)
                 DrawText(TextFormat("%ds left", oven.time_left),
                          x, ovensY + ovenTex.height + 32, FONT_XS, MAROON);
         }
 
         /* ---- staff panel (bottom) ---------------------------- */
         int by = WIN_H - BAR_H;
         DrawRectangle(0,by,WIN_W,BAR_H,(Color){235,235,235,255});
 
         DrawText("Bakers:",10,by+10,FONT_MD,DARKGRAY);
         float bx = 120 - staffScroll;
         for(int i=0;i<nBake;i++){
             Rectangle bakerSrc={0,0,bakerTex.width,bakerTex.height};
             Rectangle bakerDst={bx-32,by+15,64,96};
             DrawTexturePro(bakerTex,bakerSrc,bakerDst,(Vector2){0,0},0,WHITE);
             DrawText("State Idle",        bx+BAK_R+10,by+20,FONT_XS,BLACK);
             DrawText(TextFormat("Team T-%d",i+1), bx+BAK_R+10,by+35,FONT_XS,BLACK);
             DrawText("Item  –",          bx+BAK_R+10,by+50,FONT_XS,BLACK);
             bx += BAK_R*2 + 140;
         }
 
         DrawText("Chefs:",10,by+120,FONT_MD,DARKGRAY);
         float cx = 120 - staffScroll;
         for(int i=0;i<nChef;i++){
             float top=by+145;
             Rectangle destRec={
                 cx-(chefFrame.width*CHEF_SCALE)/2,
                 top,
                 chefFrame.width * CHEF_SCALE,
                 chefFrame.height*CHEF_SCALE
             };
             DrawTexturePro(chefTex,chefFrame,destRec,(Vector2){0,0},0,WHITE);
             DrawText("State Idle",        cx+CHEF_S/2+10,top-10,FONT_XS,BLACK);
             DrawText(TextFormat("Team C-%d",i+1),cx+CHEF_S/2+10,top+5,FONT_XS,BLACK);
             DrawText("Item  –",           cx+CHEF_S/2+10,top+20,FONT_XS,BLACK);
             cx += CHEF_S + 180;
         }
 
         if(maxScroll>0){
             float bw  = (WIN_W/contentW)*WIN_W;
             float bxBar=(staffScroll/maxScroll)*(WIN_W-bw);
             DrawRectangle(bxBar,by-8,bw,4,(Color){120,120,120,200});
         }
 
         EndDrawing();
     }
 
     /* ---------------- cleanup ----------------------------- */
     for(int i=0;i<MAX_VISUALS;i++) FreeAnimation(&visuals[i].anim);
     UnloadTexture(bakerTex); UnloadTexture(chefTex); UnloadTexture(sellerTex);
     UnloadTexture(ovenTex);  UnloadTexture(sheet);   UnloadTexture(bg);
 
     munmap(custQueue, queueShmSize(sizeof(Customer), g->config.MAX_CUSTOMERS));
     cleanup_shared_memory(g);
     CloseWindow();
     return 0;
 }
 