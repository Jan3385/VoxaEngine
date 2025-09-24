#include "Editor.h"

int main(int argc, char* argv[])
{
    GameEngine engine;

    Config::EngineConfig config;
    config.backgroundColor = RGB(67,70,75);
    config.automaticLoadingOfChunksInView = false;
    config.automaticLoadingOfChunksFromEvents = false;
    config.disableGPUSimulations = true;
    config.pauseVoxelSimulation = true;
    config.vsync = true;

    Editor editor;
    Editor::instance = editor;
    
    engine.Run(Editor::instance, config);

    return EXIT_SUCCESS;
}