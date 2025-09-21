#include "Game.h"

int main(int argc, char* argv[])
{
    GameEngine engine;

    Config::EngineConfig config;
    config.backgroundColor = RGBA(25, 196, 255, 255);
    config.vsync = false;

    Game game;
    engine.Run(game, config);

    return EXIT_SUCCESS;
}
