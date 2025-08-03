#include "GameEngine.h"
#include <iostream>
#include <chrono>

int main(int argc, char* argv[]){
    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));
    
    GameEngine::instance = new GameEngine();

    GameEngine::instance->Initialize();

    //Start simulation thread
    GameEngine::instance->StartSimulationThread();

    //Game Loop
    while (GameEngine::instance->running)
    {
        GameEngine::instance->StartFrame();

        //Update
        GameEngine::instance->Update();

        //Render
        GameEngine::instance->Render();

        GameEngine::instance->EndFrame();
    }

    delete GameEngine::instance;
    GameEngine::instance = nullptr;

    std::cout << "Thanks for playing! <3" << std::endl;

    return EXIT_SUCCESS;
}