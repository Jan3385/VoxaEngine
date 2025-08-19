#include "Game.h"

int main(int argc, char* argv[])
{
    GameEngine engine;

    EngineConfig config;
    config.backgroundColor = RGBA(25, 196, 255, 255);
    config.vsync = true;

    Game game;
    engine.Run(game, config);

    return EXIT_SUCCESS;
}
