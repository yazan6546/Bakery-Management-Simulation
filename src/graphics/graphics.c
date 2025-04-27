#include "raylib.h"
#include "animation.h"
#include <stdio.h>

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Walking Side Animation");

    Texture2D spritesheet = LoadTexture(ASSETS_PATH"characters/customer/SpriteSheet.png");

    if (spritesheet.id == 0) {
        printf("Failed to load spritesheet texture!\n");
        CloseWindow();
        return 1;
    }

    // Define the 8 frame rectangles for the walking side animation
    Rectangle walkingSideFrames[8] = {
        {22, 289, 20, 48},   // Frame 0
        {44, 289, 17, 48},   // Frame 1
        {63, 289, 12, 48},   // Frame 2
        {77, 289, 14, 48},   // Frame 3
        {97, 289, 20, 49},   // Frame 4
        {120, 289, 15, 48},   // Frame 5
        {140, 289, 13, 48},   // Frame 6
        {158, 289, 14, 48}    // Frame 7
    };

    // Create animation
    Animation* walkingSide = CreateAnimation(spritesheet, walkingSideFrames, 8, 8.0f);

    float scale = 5.0f;
    Rectangle destRec = {
        screenWidth/2.0f,
        screenHeight/2.0f,
        24 * scale,
        48 * scale
    };
    Vector2 origin = {24 * scale / 2.0f, 48 * scale / 2.0f};

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateAnimation(walkingSide, GetFrameTime());
        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawAnimationPro(walkingSide, destRec, origin, 0.0f, WHITE);
        EndDrawing();
    }

    FreeAnimation(&walkingSide);
    UnloadTexture(spritesheet);
    CloseWindow();

    return 0;
}