//
// Created by yazan on 4/27/2025.
//

#ifndef ASSETS_H
#define ASSETS_H


#include "raylib.h"


// Asset structure to organize all textures
typedef struct {
    // Customer assets
    Texture2D customer_happy;
    Texture2D customer_neutral;
    Texture2D customer_frustrated;

    // Staff assets
    Texture2D baker;
    Texture2D chef;
    Texture2D cashier;

    // Food items
    Texture2D bread;
    Texture2D cake;
    Texture2D cookie;

    // UI elements
    Font main_font;
    Sound cash_register;
} Assets;


// Global assets
extern Assets game_assets;


#endif //ASSETS_H
