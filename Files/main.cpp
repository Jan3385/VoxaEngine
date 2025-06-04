#include "GameEngine.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]){
    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));

    GameEngine gEngine;
    
    GameEngine::instance = &gEngine;
    gEngine.Player = new Game::Player(&gEngine.chunkMatrix, gEngine.renderer->LoadTexture("Textures/Player.bmp"));
    
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

int WinMain(int argc, char* argv[]){
    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));

    GameEngine gEngine;
    GameEngine::instance = &gEngine;
    gEngine.Player = new Game::Player(&gEngine.chunkMatrix, gEngine.renderer->LoadTexture("Textures/Player.bmp"));
    
    //Start simulation thread
    gEngine.StartSimulationThread();

    //Game Loop
    while (gEngine.running)
    {
        gEngine.StartFrame();

        auto updateStart = std::chrono::high_resolution_clock::now();
        //Update
        gEngine.Update();
        auto updateEnd = std::chrono::high_resolution_clock::now();
        auto updateDuration = std::chrono::duration_cast<std::chrono::microseconds>(updateEnd - updateStart).count();
        //std::cout << "Update took: " << updateDuration << " microseconds" << std::endl;

        auto renderStart = std::chrono::high_resolution_clock::now();
        //Render
        gEngine.Render();
        auto renderEnd = std::chrono::high_resolution_clock::now();
        auto renderDuration = std::chrono::duration_cast<std::chrono::microseconds>(renderEnd - renderStart).count();
        //std::cout << "Render took: " << renderDuration << " microseconds" << std::endl;

        gEngine.EndFrame();
    }

    return EXIT_SUCCESS;
}