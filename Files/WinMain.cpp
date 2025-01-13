#include "GameEngine.h"
#include <iostream>

int main(){
    // nothing
    return 0;
}

int WinMain(int argc, char* argv[]){
    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));

    GameEngine gEngine;

    //Game Loop
    while (gEngine.running)
    {
        gEngine.StartFrame();

        //Poll keyboard events
        gEngine.PollEvents();

        //Update
        gEngine.Update();

        //Render
        gEngine.Render();

        gEngine.EndFrame();
    }

    return EXIT_SUCCESS;
}