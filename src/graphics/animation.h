// animation.h
#ifndef ANIMATION_H
#define ANIMATION_H

#include "raylib.h"

// Animation struct definition
typedef struct {
    Rectangle *frames;      // Array of source rectangles for each frame
    Texture2D texture;      // Texture containing all animation frames
    float fps;              // Frames per second for this animation
    int length;             // Total number of frames
    int currentFrame;       // Current frame being displayed
    float frameTimer;       // Timer to track when to advance frames
} Animation;

// Create a new animation (allocates memory)
Animation* CreateAnimation(Texture2D texture, Rectangle *frames, int length, float fps);

// Update animation frame based on elapsed time
void UpdateAnimation(Animation *anim, float deltaTime);

// Draw current frame of animation
void DrawAnimationPro(Animation *anim, Rectangle destRec, Vector2 origin, float rotation, Color tint);

// Free animation resources
void FreeAnimation(Animation **anim);

#endif // ANIMATION_H