// animation.c
#include "animation.h"
#include <stdlib.h>

Animation* CreateAnimation(Texture2D texture, Rectangle *frames, int length, float fps) {
    Animation *anim = (Animation*)malloc(sizeof(Animation));
    if (!anim) return NULL;

    anim->texture = texture;
    anim->fps = fps;
    anim->length = length;
    anim->currentFrame = 0;
    anim->frameTimer = 0.0f;

    Rectangle *temp = (Rectangle*)malloc(length * sizeof(Rectangle));
    if (!temp) {
        TraceLog(LOG_FATAL, "Failed to allocate memory for animation frames");
        anim->length = 0;
        return anim;
    }

    anim->frames = temp;


    // Copy the frame rectangles
    for (int i = 0; i < length; i++) {
        anim->frames[i] = frames[i];
    }

    return anim;
}

void UpdateAnimation(Animation *anim, float deltaTime) {
    if (!anim) return;

    anim->frameTimer += deltaTime;

    if (anim->frameTimer >= 1.0f / anim->fps) {
        anim->frameTimer = 0.0f;
        anim->currentFrame++;

        if (anim->currentFrame >= anim->length) {
            anim->currentFrame = 0;
        }
    }
}

void DrawAnimationPro(Animation *anim, Rectangle destRec, Vector2 origin, float rotation, Color tint) {
    if (!anim || anim->length <= 0) return;

    // Create a copy of destRec with adjusted width based on the current frame
    Rectangle scaledDestRec = destRec;

    // Calculate proper scale ratio based on original frame dimensions
    float frameWidthRatio = anim->frames[anim->currentFrame].width / 24.0f;
    scaledDestRec.width = destRec.width * frameWidthRatio;

    // Recalculate origin to maintain center point
    Vector2 scaledOrigin = {
        origin.x * frameWidthRatio,
        origin.y
    };

    DrawTexturePro(
        anim->texture,
        anim->frames[anim->currentFrame],
        scaledDestRec,
        scaledOrigin,
        rotation,
        tint
    );
}

void FreeAnimation(Animation **anim) {
    if (!anim || !*anim) return;

    if ((*anim)->frames) {
        free((*anim)->frames);
    }

    free(*anim);
    *anim = NULL;
}