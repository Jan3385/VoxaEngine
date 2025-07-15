#include "GameEngine.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]){
    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));

    GameEngine gEngine;
    
    GameEngine::instance = &gEngine;

    gEngine.Initialize();
    
    //Start simulation thread
    gEngine.StartSimulationThread();

    //Game Loop
    while (gEngine.running)
    {
        gEngine.StartFrame();

        //Update
        gEngine.Update();

        //Render
        gEngine.Render();

        gEngine.EndFrame();
    }

    return EXIT_SUCCESS;
}