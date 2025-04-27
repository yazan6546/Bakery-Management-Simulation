#include "raylib.h"
#include <stdio.h>

#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (450)

#define WINDOW_TITLE "Window title"

int main(void)
{

    // Texture2D texture = LoadTexture(ASSETS_PATH"test.png"); // Check README.md for how this works

    // Raylib setup
    InitWindow(1280, 620, "Bakery Simulation");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. Draw Top Bar
        DrawRectangle(0, 0, 1280, 40, DARKGRAY);
        DrawText("Profit: $520 | Time: 12m 30s | Frustrated: 3/10", 10, 10, 20, WHITE);

        // 2. Draw Left Panel (Resources)
        DrawRectangle(0, 40, 320, 680, LIGHTGRAY);
        DrawText("Wheat: 50kg", 10, 60, 20, BLACK);
        DrawText("Salami: 0kg ❌", 10, 90, 20, RED);

        // 3. Draw Middle Panel (Teams)
        DrawRectangle(320, 40, 640, 600, WHITE);
        DrawText("Chefs - Paste: Active ✅", 330, 60, 20, DARKGREEN);

        // 4. Draw Right Panel (Customers)
        DrawRectangle(960, 40, 320, 600, LIGHTGRAY);
        DrawCircle(970, 100, 5, GREEN); // Example customer

        // 5. Draw Bottom Log
        DrawRectangle(0, 680, 1280, 40, DARKGRAY);
        DrawText("Event: Cheese restocked!", 10, 600, 20, WHITE);

        EndDrawing();

        // Update simulation state here (e.g., IPC data)
    }

    CloseWindow();

    return 0;
}
