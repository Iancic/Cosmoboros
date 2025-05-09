// USAGE INSTRUCTIONS:
// 1. Engine takes as argument a derived Program class (your game)
// 2. Your game class can override HandleInput(), Update(), Render(), Init() and Shutdown()
// 3. Implement your logic inside these 5 functions

#pragma once
#include "Engine.hpp"
#include "Game.hpp"

int main(int argc, char* argv[])
{
    Program* game = new Game("Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, false);
    game->Init();

    Engine* engine = new Engine(game);
    engine->Run();

    delete game;
    delete engine;

    return 0;
}