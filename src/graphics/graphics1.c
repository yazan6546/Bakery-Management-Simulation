//
// Created by yazan on 4/30/2025.
//
#include "raylib.h"
#include "animation.h"
#include <stdio.h>
#include <stdlib.h>

// Customer structure
typedef struct {
    Vector2 position;    // World position
    Animation* anim;     // Customer animation
    int direction;       // 1=right, -1=left
    float speed;         // Movement speed
    bool isMoving;       // Is customer moving?
} Customer;

// Cashier structure
typedef struct {
    Vector2 position;    // World position
    Animation* anim;     // Cashier animation
    bool isActive;       // Is cashier serving
} Cashier;

// Initialize a customer
Customer* CreateCustomer(Texture2D spritesheet, Rectangle frames[], int frameCount, Vector2 pos) {
    Customer* cust = (Customer*)malloc(sizeof(Customer));
    cust->anim = CreateAnimation(spritesheet, frames, frameCount, 8.0f);
    cust->position = pos;
    cust->direction = 1;
    cust->speed = 2.0f;
    cust->isMoving = false;
    return cust;
}

// Initialize a cashier
Cashier* CreateCashier(Texture2D spritesheet, Rectangle frames[], int frameCount, Vector2 pos) {
    Cashier* cashier = (Cashier*)malloc(sizeof(Cashier));
    cashier->anim = CreateAnimation(spritesheet, frames, frameCount, 8.0f);
    cashier->position = pos;
    cashier->isActive = true;
    return cashier;
}

void FreeCustomer(Customer* cust) {
    FreeAnimation(&cust->anim);
    free(cust);
}

void FreeCashier(Cashier* cashier) {
    FreeAnimation(&cashier->anim);
    free(cashier);
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Bakery Simulation - Infinite Counter");

    // Load background texture
    Texture2D background = LoadTexture(ASSETS_PATH"Background3.png");
    if (background.id == 0) {
        printf("Failed to load background texture!\n");
        CloseWindow();
        return 1;
    }

    // Background properties
    int backgroundWidth = background.width;
    int backgroundHeight = background.height;
    float bgScale = (float)screenHeight / backgroundHeight;

    // Define counter region (bottom part of the background)
    float counterHeight = backgroundHeight * 0.25f;
    float counterY = backgroundHeight * 0.75f;
    Rectangle counterRegion = {
        0,                  // x
        counterY,           // y
        backgroundWidth,    // width
        counterHeight       // height
    };

    // Load customer spritesheet
    Texture2D spritesheet = LoadTexture(ASSETS_PATH"characters/customer/SpriteSheet.png");
    if (spritesheet.id == 0) {
        printf("Failed to load spritesheet texture!\n");
        UnloadTexture(background);
        CloseWindow();
        return 1;
    }

    // Define the frame rectangles for walking animation
    Rectangle walkingSideFrames[8] = {
        {22, 289, 20, 48},   // Frame 0
        {44, 289, 17, 48},   // Frame 1
        {63, 289, 12, 48},   // Frame 2
        {77, 289, 14, 48},   // Frame 3
        {97, 289, 20, 49},   // Frame 4
        {120, 289, 15, 48},  // Frame 5
        {140, 289, 13, 48},  // Frame 6
        {158, 289, 14, 48}   // Frame 7
    };

    // Define cashier frames
    Rectangle cashierFrames[1] = {
        {97, 289, 20, 49}  // Using a single frame for cashier
    };

    // World properties
    float worldLimit = 10000.0f;  // Practical limit for our infinite world
    float characterScale = 5.0f;
    float counterWidthScaled = backgroundWidth * bgScale;

    // Number of counter segments in the world
    int totalCounterSegments = 10;

    // Calculate the position of the first (rightmost) tile
    float firstTileX = worldLimit - counterWidthScaled;

    // Start camera at the rightmost tile
    Camera2D camera = { 0 };
    camera.target.x = firstTileX;  // Start at the right end
    camera.target.y = 0;
    camera.offset = (Vector2){ 0, 0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Position player on the right side of the first tile
    Customer* player = CreateCustomer(spritesheet, walkingSideFrames, 8,
                                     (Vector2){firstTileX + counterWidthScaled/2, screenHeight - 150});
    player->speed = 5.0f;
    player->direction = -1;  // Facing left initially

    // Create some example customers
    const int maxCustomers = 20;
    Customer* customers[20] = { 0 };
    for (int i = 0; i < maxCustomers; i++) {
        // Position customers from right to left
        customers[i] = CreateCustomer(spritesheet, walkingSideFrames, 8,
                                     (Vector2){firstTileX + (i+1) * 100, screenHeight - 150});
        customers[i]->direction = -1;  // Facing left
    }

    // Create cashiers on the first (rightmost) counter tile
    const int maxCashiers = 3;
    Cashier* cashiers[3] = { 0 };
    for (int i = 0; i < maxCashiers; i++) {
        // Position cashiers evenly across the rightmost counter
        float spacing = counterWidthScaled / (maxCashiers + 1);
        float cashierX = firstTileX + (i + 1) * spacing;
        cashiers[i] = CreateCashier(spritesheet, cashierFrames, 1,
                                  (Vector2){cashierX, screenHeight - 180});
    }

    // Camera control speed
    float cameraSpeed = 8.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Handle camera movement only with arrow keys
        if (IsKeyDown(KEY_RIGHT)) {
            camera.target.x += cameraSpeed;
        }
        else if (IsKeyDown(KEY_LEFT)) {
            camera.target.x -= cameraSpeed;
        }

        // Keep camera within world bounds
        if (camera.target.x < 0) camera.target.x = 0;
        if (camera.target.x > firstTileX) camera.target.x = firstTileX;

        // Simulate player movement with A and D keys
        player->isMoving = false;
        if (IsKeyDown(KEY_D)) {
            player->position.x += player->speed;
            player->isMoving = true;
            player->direction = 1;  // Right
        }
        else if (IsKeyDown(KEY_A)) {
            player->position.x -= player->speed;
            player->isMoving = true;
            player->direction = -1; // Left
        }

        // Update player animation
        if (player->isMoving) {
            UpdateAnimation(player->anim, GetFrameTime());
        }

        // Update customer animations and movement
        for (int i = 0; i < maxCustomers; i++) {
            if (GetRandomValue(0, 100) < 3) {
                customers[i]->direction *= -1;
            }

            if (GetRandomValue(0, 100) < 5) {
                customers[i]->isMoving = !customers[i]->isMoving;
            }

            if (customers[i]->isMoving) {
                customers[i]->position.x += customers[i]->speed * customers[i]->direction;
                UpdateAnimation(customers[i]->anim, GetFrameTime());
            }
        }

        // Update cashier animations
        for (int i = 0; i < maxCashiers; i++) {
            UpdateAnimation(cashiers[i]->anim, GetFrameTime());
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Calculate the number of visible segments
            int tilesNeeded = (screenWidth / (counterWidthScaled)) + 2;

            // Draw the main background on the RIGHTMOST tile
            Rectangle bgSource = { 0, 0, backgroundWidth, backgroundHeight };
            Rectangle bgDest = {
                firstTileX - camera.target.x,  // Position at the rightmost tile
                0,
                backgroundWidth * bgScale,
                screenHeight
            };
            DrawTexturePro(background, bgSource, bgDest, (Vector2){0, 0}, 0.0f, WHITE);

            // Draw repeating counter segments from right to left
            for (int i = 0; i < totalCounterSegments; i++) {
                // Position segments from right to left (0 is rightmost)
                float segmentX = firstTileX - i * counterWidthScaled;

                // Skip drawing counter on rightmost tile since full background is there
                if (i == 0) continue;

                // Only draw the counter portion of subsequent backgrounds
                Rectangle counterSource = counterRegion;

                // Position counter segments side by side
                Rectangle counterDest = {
                    segmentX - camera.target.x,
                    screenHeight - (counterHeight * bgScale),
                    counterWidthScaled,
                    counterHeight * bgScale
                };

                // Only draw if this segment would be visible
                if (counterDest.x + counterDest.width > 0 && counterDest.x < screenWidth) {
                    // Draw the counter segment
                    DrawTexturePro(background, counterSource, counterDest, (Vector2){0, 0}, 0.0f, WHITE);

                    // For debugging - draw segment index
                    DrawText(TextFormat("Segment %d", i), counterDest.x + 10, counterDest.y + 10, 20, RED);
                }
            }

            // Draw cashiers on the first (rightmost) counter segment
            for (int i = 0; i < maxCashiers; i++) {
                // Calculate screen position
                float screenX = cashiers[i]->position.x - camera.target.x;

                // Only draw if on screen (with some margin)
                if (screenX > -100 && screenX < screenWidth + 100) {
                    Rectangle cashierFrameRect = cashierFrames[cashiers[i]->anim->currentFrame];
                    float cashierFrameWidth = cashierFrameRect.width;
                    float cashierFrameHeight = cashierFrameRect.height;

                    Rectangle cashierDestRec = {
                        screenX,
                        cashiers[i]->position.y,
                        cashierFrameWidth * characterScale,
                        cashierFrameHeight * characterScale
                    };

                    Vector2 cashierOrigin = {
                        cashierFrameWidth * characterScale / 2.0f,
                        cashierFrameHeight * characterScale / 2.0f
                    };

                    // Draw cashier with unique color
                    Color cashierTint = (Color){255, 200 - i*40, 200 - i*40, 255};
                    DrawAnimationPro(cashiers[i]->anim, cashierDestRec, cashierOrigin, 0.0f, cashierTint);

                    // Draw a counter sign above each cashier
                    DrawRectangle(screenX - 20, cashiers[i]->position.y - 80, 40, 20, BLACK);
                    DrawText(TextFormat("%d", i+1), screenX - 5, cashiers[i]->position.y - 77, 16, WHITE);
                }
            }

            // Draw player character
            Rectangle frameRect = walkingSideFrames[player->anim->currentFrame];
            float frameWidth = frameRect.width;
            float frameHeight = frameRect.height;

            Rectangle destRec = {
                player->position.x - camera.target.x,
                player->position.y,
                frameWidth * characterScale,
                frameHeight * characterScale
            };

            Vector2 origin = {
                frameWidth * characterScale / 2.0f,
                frameHeight * characterScale / 2.0f
            };

            if (player->direction > 0) {
                DrawAnimationPro(player->anim, destRec, origin, 0.0f, WHITE);
            } else {
                Rectangle flippedDestRec = destRec;
                flippedDestRec.width *= -1;  // Flip horizontally
                DrawAnimationPro(player->anim, flippedDestRec, origin, 0.0f, WHITE);
            }

            // Draw customers - only if in visible area
            for (int i = 0; i < maxCustomers; i++) {
                // Calculate screen position
                float screenX = customers[i]->position.x - camera.target.x;

                // Only draw if on screen (with some margin)
                if (screenX > -100 && screenX < screenWidth + 100) {
                    Rectangle custFrameRect = walkingSideFrames[customers[i]->anim->currentFrame];
                    float custFrameWidth = custFrameRect.width;
                    float custFrameHeight = custFrameRect.height;

                    Rectangle custDestRec = {
                        screenX,
                        customers[i]->position.y,
                        custFrameWidth * characterScale,
                        custFrameHeight * characterScale
                    };

                    Vector2 custOrigin = {
                        custFrameWidth * characterScale / 2.0f,
                        custFrameHeight * characterScale / 2.0f
                    };

                    if (customers[i]->direction > 0) {
                        DrawAnimationPro(customers[i]->anim, custDestRec, custOrigin, 0.0f, WHITE);
                    } else {
                        Rectangle flippedCustDestRec = custDestRec;
                        flippedCustDestRec.width *= -1;  // Flip horizontally
                        DrawAnimationPro(customers[i]->anim, flippedCustDestRec, custOrigin, 0.0f, WHITE);
                    }
                }
            }

            // Draw HUD information
            DrawRectangle(10, 10, 290, 70, (Color){0, 0, 0, 120});
            DrawText(TextFormat("Player X: %.0f", player->position.x), 20, 20, 20, WHITE);
            DrawText(TextFormat("Camera X: %.0f", camera.target.x), 20, 45, 20, WHITE);
            DrawText("Controls: Arrows=Camera, A/D=Player", 20, 70, 15, WHITE);

        EndDrawing();
    }

    // Cleanup
    FreeCustomer(player);
    for (int i = 0; i < maxCustomers; i++) {
        if (customers[i]) FreeCustomer(customers[i]);
    }

    for (int i = 0; i < maxCashiers; i++) {
        if (cashiers[i]) FreeCashier(cashiers[i]);
    }

    UnloadTexture(spritesheet);
    UnloadTexture(background);
    CloseWindow();

    return 0;
}