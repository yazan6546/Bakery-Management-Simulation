//
// Created by yazan on 4/27/2025.
//
#include "assets.h"
Assets game_assets;

// Load all assets at startup
void load_assets() {
    // Load customer textures
    game_assets.customer_happy = LoadTexture("assets/customer_happy.png");
    game_assets.customer_neutral = LoadTexture("assets/customer_neutral.png");
    game_assets.customer_frustrated = LoadTexture("assets/customer_frustrated.png");

    // Load staff textures
    game_assets.baker = LoadTexture("assets/baker.png");
    game_assets.chef = LoadTexture("assets/chef.png");
    game_assets.cashier = LoadTexture("assets/cashier.png");

    // Load food textures
    game_assets.bread = LoadTexture("assets/bread.png");
    game_assets.cake = LoadTexture("assets/cake.png");
    game_assets.cookie = LoadTexture("assets/cookie.png");

    // Load UI elements
    game_assets.main_font = LoadFont("assets/fonts/arial.ttf");
    game_assets.cash_register = LoadSound("assets/sounds/cash_register.wav");
}

// Unload all assets at cleanup
void unload_assets() {
    // Unload customer textures
    UnloadTexture(game_assets.customer_happy);
    UnloadTexture(game_assets.customer_neutral);
    UnloadTexture(game_assets.customer_frustrated);

    // Unload staff textures
    UnloadTexture(game_assets.baker);
    UnloadTexture(game_assets.chef);
    UnloadTexture(game_assets.cashier);

    // Unload food textures
    UnloadTexture(game_assets.bread);
    UnloadTexture(game_assets.cake);
    UnloadTexture(game_assets.cookie);

    // Unload UI elements
    UnloadFont(game_assets.main_font);
    UnloadSound(game_assets.cash_register);
}
