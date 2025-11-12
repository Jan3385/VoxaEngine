#include "Editor.h"

int main(int argc, char* argv[])
{
    Debug::Logger::Instance().minLogLevel = Debug::Logger::Level::SPAM;

    GameEngine engine;

    Config::EngineConfig config;
    config.backgroundColor = RGB(67,70,75);
    config.automaticLoadingOfChunksInView = false;
    config.automaticLoadingOfChunksFromEvents = false;
    config.disableGPUSimulations = true;
    config.pauseVoxelSimulation = true;
    config.voxelFixedDeltaTime = 1.0f / 30.0f;
    config.vsync = true;

    Editor editor;
    Editor::instance = editor;
    
    engine.Run(Editor::instance, config);

    return EXIT_SUCCESS;
}