#include "Game.h"

int main(int argc, char* argv[])
{
    GameEngine engine;

    Config::EngineConfig config;
    config.backgroundColor = RGB(25, 196, 255);
    config.vsync = false;
    config.consoleTimerWarnings = true;

    Game game;
    engine.Run(game, config);

    return EXIT_SUCCESS;
}
