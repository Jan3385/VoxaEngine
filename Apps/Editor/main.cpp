#include "Editor.h"

int main(int argc, char* argv[])
{
    GameEngine engine;

    Config::EngineConfig config;
    config.backgroundColor = RGB(85, 85, 85);
    config.automaticLoadingOfChunksInView = false;
    config.disableGPUSimulations = true;
    config.pauseVoxelSimulation = true;
    config.vsync = true;

    Game game;
    engine.Run(game, config);

    return EXIT_SUCCESS;
}