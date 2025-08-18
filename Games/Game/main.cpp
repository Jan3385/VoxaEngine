#include <iostream>
#include <GameEngine.h>

class Game : public IGame {
    void OnInitialize() override {
        
    }
    void OnShutdown() override {
        
    }
    void Update() override {
        
    }
    void FixedUpdate() override {
        
    }
    void PhysicsUpdate() override {
        
    }
    void Render() override {
        
    }
};

int main(int argc, char* argv[])
{
    GameEngine engine;

    EngineConfig config;
    engine.Initialize(config);

    Game game;
    engine.Run(game, config);

    return EXIT_SUCCESS;
}