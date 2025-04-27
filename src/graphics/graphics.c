#include "raylib.h"
#include "animation.h"
#include <stdio.h>

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Walking Side Animation");

    // Load background texture
    Texture2D background = LoadTexture(ASSETS_PATH"Background2.jpeg");
    if (background.id == 0) {
        printf("Failed to load background texture!\n");
        CloseWindow();
        return 1;
    }

    Texture2D spritesheet = LoadTexture(ASSETS_PATH"characters/customer/SpriteSheet.png");
    if (spritesheet.id == 0) {
        printf("Failed to load spritesheet texture!\n");
        UnloadTexture(background);
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
        {120, 289, 15, 48},  // Frame 5
        {140, 289, 13, 48},  // Frame 6
        {158, 289, 14, 48}   // Frame 7
    };

    // Create animation
    Animation* walkingSide = CreateAnimation(spritesheet, walkingSideFrames, 8, 8.0f);

    // Character position and movement variables
    Vector2 position = { screenWidth/2.0f, screenHeight/2.0f };
    float speed = 5.0f;
    bool isMoving = false;
    int direction = 1;  // 1 for right, -1 for left

    float scale = 5.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Process input for movement (only left/right)
        isMoving = false;

        if (IsKeyDown(KEY_RIGHT)) {
            position.x += speed;
            isMoving = true;
            direction = 1;  // Moving right
        }
        else if (IsKeyDown(KEY_LEFT)) {
            position.x -= speed;
            isMoving = true;
            direction = -1; // Moving left
        }

        // Keep character within screen bounds
        float characterWidth = walkingSideFrames[walkingSide->currentFrame].width * scale / 2.0f;
        if (position.x < characterWidth) position.x = characterWidth;
        if (position.x > screenWidth - characterWidth) position.x = screenWidth - characterWidth;

        // Only update animation if moving
        if (isMoving) {
            UpdateAnimation(walkingSide, GetFrameTime());
        }

        // Get current frame width for proper proportions
        float currentFrameWidth = walkingSideFrames[walkingSide->currentFrame].width;
        float currentFrameHeight = walkingSideFrames[walkingSide->currentFrame].height;

        // Update destination rectangle
        Rectangle destRec = {
            position.x,
            position.y,
            currentFrameWidth * scale,
            currentFrameHeight * scale
        };

        // Calculate origin for proper centering
        Vector2 origin = {
            currentFrameWidth * scale / 2.0f,
            currentFrameHeight * scale / 2.0f
        };

        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Draw background first
            DrawTexture(background, 0, 0, WHITE);

        // Select the correct orientation based on direction
        if (direction > 0) {
            // Draw facing right (original orientation)
            DrawAnimationPro(walkingSide, destRec, origin, 0.0f, WHITE);
        } else {
            // Mirror the sprite for walking left
            Rectangle flippedDestRec = destRec;
            flippedDestRec.width *= -1;  // Flip horizontally by negating width
            DrawAnimationPro(walkingSide, flippedDestRec, origin, 0.0f, WHITE);
        }

        EndDrawing();
    }

    FreeAnimation(&walkingSide);
    UnloadTexture(spritesheet);
    UnloadTexture(background);
    CloseWindow();

    return 0;
}