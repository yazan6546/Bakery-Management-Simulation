/****************************************************************
 *  graphics.c  –  queue‑driven customer rendering
 *                 + diagonal “frustrated walk‑off”
 *                 + frustration sound effect
 ****************************************************************/
 #include "raylib.h"
 #include "raymath.h"          // Vector math helpers
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
 
 /* ---------- window & layout --------------------------------- */
 #ifndef ASSETS_PATH
 #define ASSETS_PATH "assets/"
 #endif
 #define WIN_W   1700
 #define WIN_H   1000
 #define DASH_W   520
 #define BAR_H    240
 #define TOP_M      0
 #define DRAW_SCALE 0.90f
 
 /* ---------- fonts ------------------------------------------- */
 #define FONT_LG 26
 #define FONT_MD 18
 #define FONT_SM 14
 #define FONT_XS 11
 
 /* ---------- customer visuals -------------------------------- */
 #define MAX_VISUALS   64          // Max customers shown
 #define MAX_LEAVERS   64          // Max frustrated customers in exit animation
 #define QUEUE_SPACING 68
 #define WALK_SCALE    5.0f
 #define LEAVE_SPEED   180.0f      // px/s exit speed
 
 /* ---------- asset helper ------------------------------------ */
 static Texture2D mustLoad(const char *p) {
     Texture2D t = LoadTexture(p);
     if (!t.id) {
         TraceLog(LOG_FATAL, "cannot load %s", p);
         exit(1);
     }
     return t;
 }
 
 /* ---------- tint + state label ------------------------------ */
 static Color tintForState(CustomerState s) {
     switch (s) {
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
 
 static const char* nameForState(CustomerState s) {
     switch (s) {
         case WALKING:            return "Walking";
         case WAITING_IN_QUEUE:   return "Queue";
         case WAITING_FOR_ORDER:  return "Waiting";
         case ORDERING:           return "Ordering";
         case FRUSTRATED:         return "Frustrated";
         case COMPLAINING:        return "Complaining";
         case CONTAGION:          return "Contagion";
         default:                 return "?";
     }
 }
 
 /* ---------- sprite frame tables ----------------------------- */
 static const Rectangle walkFrames[8] = {
     {22,289,20,48}, {44,289,17,48}, {63,289,12,48}, {77,289,14,48},
     {97,289,20,49}, {120,289,15,48}, {140,289,13,48}, {158,289,14,48}
 };
 
 static const Rectangle frustFrames[4] = {
     {0,0,20,48}, {20,0,20,48}, {40,0,20,48}, {60,0,20,48}
 };
 
 static const int idleFrameIdx = 0;
 
 /* ---------- per‑queue visual slot --------------------------- */
 typedef struct {
     Animation     *anim;
     CustomerState  lastState;
 } Visual;
 
 static Visual visuals[MAX_VISUALS] = {0};
 
 static void ensureAnim(Texture2D sheet, Visual *v, CustomerState st) {
     const Rectangle *set = walkFrames; int len = 8; float fps = 8.0f;
     if (st == FRUSTRATED) { set = frustFrames; len = 4; fps = 12.0f; }
     if (!v->anim || v->lastState != st) {
         FreeAnimation(&v->anim);
         v->anim = CreateAnimation(sheet, (Rectangle*)set, len, fps);
         v->lastState = st;
     }
 }
 
 /* ---------- “Leaver” for frustrated walk‑off ---------------- */
 typedef struct {
     const Customer *ref;
     Animation *anim;
     float x, y;
     float dx, dy;
     float tgtX, tgtY;
     bool active;
 } Leaver;
 
 static Leaver leavers[MAX_LEAVERS] = {0};
 static float EXIT_X = 0, EXIT_Y = 0;
 
 /* ---------- sound effect ----------------------------------- */
 static Sound frustratedSound = {0};
 
 static void playFrustrationSound() {
     if (frustratedSound.frameCount > 0) {
         PlaySound(frustratedSound);
     }
 }
 
 /* create leaver if free slot & not already present             */
 static void addLeaver(Texture2D sheet, const Customer *ref, float startX, float startY) {
     for (int i = 0; i < MAX_LEAVERS; i++) {
         if (leavers[i].active && leavers[i].ref == ref) return;
     }
     for (int i = 0; i < MAX_LEAVERS; i++) {
         if (!leavers[i].active) {
             leavers[i].ref = ref;
             leavers[i].x = startX;
             leavers[i].y = startY;
             leavers[i].tgtX = WIN_W + 100;  // Off-screen to the right
             leavers[i].tgtY = WIN_H - 100;
             Vector2 dir = Vector2Normalize((Vector2){ EXIT_X - startX, EXIT_Y - startY });
             leavers[i].dx = dir.x;
             leavers[i].dy = dir.y;
             leavers[i].anim = CreateAnimation(sheet, (Rectangle*)frustFrames, 4, 12.0f);
             leavers[i].active = true;
             
 
             // 🔊 Play frustration sound once when customer starts leaving
             playFrustrationSound();
             break;
         }
     }
 }
 
 /* ============================================================ */
 int main(void) {
     Game *g = NULL; setup_shared_memory(&g);
     queue_shm *custQ = NULL;
     setup_queue_shared_memory(&custQ, g->config.MAX_CUSTOMERS);
 
     /* -------- load textures --------------------------------- */
     InitWindow(WIN_W, WIN_H, "Bakery GUI");
     Texture2D bg   = mustLoad(ASSETS_PATH "backgroundnew.png");
     Texture2D sheet= mustLoad(ASSETS_PATH "characters/customer/SpriteSheet.png");
     Texture2D oven = mustLoad(ASSETS_PATH "oven.png");
     Texture2D chef = mustLoad(ASSETS_PATH "characters/chef/ChefSheet.png");
     Texture2D seller = mustLoad(ASSETS_PATH "seller.png");
     Texture2D baker  = mustLoad(ASSETS_PATH "baker.png");
 
     // 🔊 Load frustration sound
     InitAudioDevice();
     frustratedSound = LoadSound(ASSETS_PATH "frustrated.wav");
     if (frustratedSound.frameCount == 0) {
         TraceLog(LOG_WARNING, "Could not load frustration sound");
     }
 
     // Static sprite sizes
     const float SELL_SCALE = 0.12f;
     const float SELL_W = seller.width * SELL_SCALE;
     const float SELL_H = seller.height * SELL_SCALE;
     const Rectangle chefFrame = {7,2,32,52};
     const float CHEF_SCALE = 2.0f, BAK_R = 30, CHEF_S = 60;
 
     // World geometry
     const int BG_W = WIN_W - DASH_W;
     const int BG_H = WIN_H - BAR_H;
     const float SCALE = (BG_H / (float)bg.height) * DRAW_SCALE;
     const float WORLD_W = bg.width * SCALE;
     const float WORLD_H = bg.height * SCALE;
     const float drawH   = WORLD_H;
     const float groundY = TOP_M + drawH - walkFrames[0].height * WALK_SCALE / 2;
     const float sellerY = TOP_M + drawH - 200;
 
     // Exit coordinates for frustrated customers
     EXIT_X = WIN_W + 100;  // Fully off-screen
     EXIT_Y = WIN_H - 100;  // Bottom-right corner
 
     // Camera & scrolling
     Camera2D cam = {0};
     float vScroll = 0, staffScroll = 0;
 
     while (!WindowShouldClose()) {
         float dt = GetFrameTime();
 
         // Camera controls
         if (IsKeyDown(KEY_RIGHT)) cam.target.x += 8;
         if (IsKeyDown(KEY_LEFT )) cam.target.x -= 8;
         cam.target.x = Clamp(cam.target.x, 0, WORLD_W - BG_W);
         if (IsKeyDown(KEY_DOWN)) vScroll += 8;
         if (IsKeyDown(KEY_UP  )) vScroll -= 8;
         if (vScroll < 0) vScroll = 0;
 
         // Staff bar scrolling
         if (GetMouseY() > WIN_H - BAR_H)
             staffScroll -= GetMouseWheelMove() * 40;
         if (IsKeyDown(KEY_LEFT_BRACKET )) staffScroll -= 8;
         if (IsKeyDown(KEY_RIGHT_BRACKET)) staffScroll += 8;
         float bakerW = 120 + g->config.NUM_BAKERS * (BAK_R * 2 + 140);
         float chefW  = 120 + g->config.NUM_CHEFS * (CHEF_S + 180);
         float contentW = bakerW > chefW ? bakerW : chefW;
         float maxScroll = contentW > WIN_W ? contentW - WIN_W : 0;
         staffScroll = Clamp(staffScroll, 0, maxScroll);
 
         BeginDrawing();
         ClearBackground(RAYWHITE);
 
         // === world & queue ===================================
         BeginScissorMode(0, 0, BG_W, BG_H);
         {
             Rectangle src={0,0,bg.width,bg.height};
             Rectangle dst={-cam.target.x,TOP_M-vScroll,bg.width*SCALE,drawH};
             DrawTexturePro(bg, src, dst, (Vector2){0,0}, 0, WHITE);
 
             // Draw sellers
             float sp = WORLD_W / (g->config.NUM_SELLERS + 1.0f);
             for (int i = 0; i < g->config.NUM_SELLERS; i++) {
                 float x = -cam.target.x + sp*(i+1) - SELL_W/2;
                 Rectangle sSrc = {0,0,(float)seller.width,(float)seller.height};
                 Rectangle sDst = {x, sellerY-SELL_H-vScroll-40, SELL_W, SELL_H};
                 DrawTexturePro(seller, sSrc, sDst, (Vector2){0,0}, 0, WHITE);
             }
 
             // Draw queue customers & spawn leavers
             int visCnt = custQ->count > MAX_VISUALS ? MAX_VISUALS : custQ->count;
             for (int i = 0; i < visCnt; i++) {
                 int idx = (custQ->head + i) % custQ->capacity;
                 Customer *c = &((Customer*)custQ->elements)[idx];
 
                 if (c->state == FRUSTRATED) {
                     float startX = sp - SELL_W/2 - 60 - i * QUEUE_SPACING;
                     addLeaver(sheet, c, startX, groundY);
                     continue;
                 }
 
                 Visual *v = &visuals[i];
                 ensureAnim(sheet, v, c->state);
                 UpdateAnimation(v->anim, dt);
                 if (c->state != WALKING) v->anim->currentFrame = idleFrameIdx;
 
                 float posX = sp - SELL_W/2 - 60 - i * QUEUE_SPACING;
                 Rectangle fr = v->anim->frames[v->anim->currentFrame];
                 float w = fr.width * WALK_SCALE, h = fr.height * WALK_SCALE;
                 Rectangle dr = {posX - cam.target.x, groundY - vScroll, w, h};
                 Vector2 org = {w/2, h/2};
                 DrawAnimationPro(v->anim, dr, org, 0, tintForState(c->state));
                 DrawText(nameForState(c->state), (int)(dr.x - 20), (int)(dr.y - h/2 - 14), FONT_XS, BLACK);
             }
 
             // Update & draw leavers (frustrated customers)
             for (int i = 0; i < MAX_LEAVERS; i++) {
                 if (!leavers[i].active) continue;
 
                 Animation *a = leavers[i].anim;
                 UpdateAnimation(a, dt);
                 leavers[i].x += leavers[i].dx * LEAVE_SPEED * dt;
                 leavers[i].y += leavers[i].dy * LEAVE_SPEED * dt;
 
                 Rectangle fr = a->frames[a->currentFrame];
                 float w = fr.width * WALK_SCALE, h = fr.height * WALK_SCALE;
                 Rectangle dr = {leavers[i].x - cam.target.x, leavers[i].y - vScroll, w, h};
                 Vector2 org = {w/2, h/2};
                 DrawAnimationPro(a, dr, org, 0, RED);
                 DrawText("Leaving", (int)(dr.x - 20), (int)(dr.y - h/2 - 14), FONT_XS, BLACK);
 
                 // Cull if fully off-screen
                 if (leavers[i].x > WIN_W + 100) {
                     leavers[i].active = false;
                     FreeAnimation(&leavers[i].anim);
                 }
             }
         }
         EndScissorMode();
 
         // === right dashboard =================================
         int rx = BG_W;
         DrawRectangle(rx, 0, DASH_W, WIN_H, (Color){245, 245, 245, 255});
         DrawText("Game Dashboard", rx + 10, 10, FONT_LG, DARKGRAY);
         DrawText(TextFormat("Time  : %d s", g->elapsed_time), rx + 10, 50, FONT_MD, BLACK);
         DrawText(TextFormat("Profit: %.2f" , g->daily_profit), rx + 10, 70, FONT_MD, BLACK);
         int y = 100;
         DrawText("Customers:", rx + 10, y, FONT_MD, DARKGRAY); y += 20;
         DrawText(TextFormat("Served        : %d", g->num_customers_served), rx + 10, y, FONT_XS, BLACK); y += 14;
         DrawText(TextFormat("Frustrated    : %d", g->num_frustrated_customers), rx + 10, y, FONT_XS, BLACK); y += 14;
         DrawText(TextFormat("Complained    : %d", g->num_complained_customers), rx + 10, y, FONT_XS, BLACK); y += 14;
         DrawText(TextFormat("Missing Item  : %d", g->num_customers_missing), rx + 10, y, FONT_XS, BLACK); y += 14;
         DrawText(TextFormat("Cascade Events: %d", g->num_customers_cascade), rx + 10, y, FONT_XS, BLACK); y += 20;
 
         // Ready products & ingredients
         int colW = (DASH_W - 20) / 2, readyX = rx + 10, ingrX = rx + 10 + colW;
         int yR = y, yI = y;
         DrawText("Ready products:", readyX, yR, FONT_MD, DARKGRAY); yR += 20;
         DrawText("Ingredients:", ingrX, yI, FONT_MD, DARKGRAY); yI += 20;
 
         for (int cat = 0; cat < NUM_PRODUCTS; cat++) {
             const ProductCategory *pc = &g->productCatalog.categories[cat];
             if (pc->product_count == 0) continue;
             DrawText((const char*[]){"Bread","Cake","Sandwich","Sweet","SweetPat.","SavoryPat."}[cat],
                      readyX, yR, FONT_SM, MAROON); yR += 16;
             for (int p = 0; p < pc->product_count; p++) {
                 int q = g->ready_products.categories[cat].quantities[p];
                 DrawText(TextFormat("\u2022 %s: %d", pc->products[p].name, q),
                          readyX + 12, yR, FONT_XS, BLACK); yR += 14;
                 if (yR > WIN_H - BAR_H - 160) {
                     DrawText("...", readyX + 12, yR, FONT_XS, BLACK);
                     goto SKIP_PROD;
                 }
             }
         }
 SKIP_PROD:
         for (int ing = 0; ing < NUM_INGREDIENTS; ing++) {
             float q = g->inventory.quantities[ing];
             DrawText(TextFormat("%s: %.1f", get_ingredient_name(ing), q),
                      ingrX, yI, FONT_XS, BLACK); yI += 14;
             if (yI > WIN_H - BAR_H - 160) {
                 DrawText("...", ingrX, yI, FONT_XS, BLACK);
                 break;
             }
         }
 
         // Ovens
         float ovensY = yI + 20;
         if (ovensY + oven.height > WIN_H - BAR_H - 10)
             ovensY = WIN_H - BAR_H - oven.height - 10;
         int ovensX = rx + 10;
         for (int o = 0; o < g->config.NUM_OVENS; o++) {
             int x = ovensX + o * (oven.width + 40);
             DrawTexture(oven, x, ovensY, WHITE);
             Oven ov = g->ovens[o];
             DrawText(ov.is_busy ? "Preparing" : "Idle",
                      x, ovensY + oven.height + 4, FONT_XS,
                      ov.is_busy ? RED : DARKGREEN);
             DrawText(ov.item_name, x, ovensY + oven.height + 18, FONT_XS, BLACK);
             if (ov.is_busy)
                 DrawText(TextFormat("%ds left", ov.time_left),
                          x, ovensY + oven.height + 32, FONT_XS, MAROON);
         }
 
         // Staff panel
         int by = WIN_H - BAR_H;
         DrawRectangle(0, by, WIN_W, BAR_H, (Color){235, 235, 235, 255});
         DrawText("Bakers:", 10, by + 10, FONT_MD, DARKGRAY);
         float bx = 120 - staffScroll;
         for (int i = 0; i < g->config.NUM_BAKERS; i++) {
             Rectangle src = {0, 0, baker.width, baker.height};
             Rectangle dst = {bx - 32, by + 15, 64, 96};
             DrawTexturePro(baker, src, dst, (Vector2){0, 0}, 0, WHITE);
             DrawText("State Idle", bx + BAK_R + 10, by + 20, FONT_XS, BLACK);
             DrawText(TextFormat("Team T-%d", i + 1), bx + BAK_R + 10, by + 35, FONT_XS, BLACK);
             DrawText("Item  –", bx + BAK_R + 10, by + 50, FONT_XS, BLACK);
             bx += BAK_R * 2 + 140;
         }
 
         DrawText("Chefs:", 10, by + 120, FONT_MD, DARKGRAY);
         float cx = 120 - staffScroll;
         for (int i = 0; i < g->config.NUM_CHEFS; i++) {
             float top = by + 145;
             Rectangle dst = {
                 cx - (chefFrame.width * CHEF_SCALE) / 2,
                 top,
                 chefFrame.width * CHEF_SCALE,
                 chefFrame.height * CHEF_SCALE
             };
             DrawTexturePro(chef, chefFrame, dst, (Vector2){0, 0}, 0, WHITE);
             DrawText("State Idle", cx + CHEF_S / 2 + 10, top - 10, FONT_XS, BLACK);
             DrawText(TextFormat("Team C-%d", i + 1), cx + CHEF_S / 2 + 10, top + 5, FONT_XS, BLACK);
             DrawText("Item  –", cx + CHEF_S / 2 + 10, top + 20, FONT_XS, BLACK);
             cx += CHEF_S + 180;
         }
 
         if (maxScroll > 0) {
             float bw = (WIN_W / contentW) * WIN_W;
             float bxBar = (staffScroll / maxScroll) * (WIN_W - bw);
             DrawRectangle(bxBar, by - 8, bw, 4, (Color){120, 120, 120, 200});
         }
 
         EndDrawing();
     }
 
     // Cleanup
     for (int i = 0; i < MAX_VISUALS; i++) FreeAnimation(&visuals[i].anim);
     for (int i = 0; i < MAX_LEAVERS; i++) {
         if (leavers[i].active) FreeAnimation(&leavers[i].anim);
     }
     UnloadTexture(baker); UnloadTexture(chef); UnloadTexture(seller);
     UnloadTexture(oven);  UnloadTexture(sheet); UnloadTexture(bg);
 
     // 🔊 Unload sound
     UnloadSound(frustratedSound);
     CloseAudioDevice();
 
     munmap(custQ, queueShmSize(sizeof(Customer), g->config.MAX_CUSTOMERS));
     cleanup_shared_memory(g);
     CloseWindow();
     return 0;
 }