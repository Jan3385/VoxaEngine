### Engine

The engine is the main class handling all relevant data and program runtime.
Your program's runtime should be only handled via the `IGame` struct implementation passed on during the `GameEngine::Run` method which should be only called once at the start of the program.

The engine is configured by the `EngineConfig` struct, which is also passed during the `GameEngine::Run` call

Minimal example for the engine startup inside `main` 

```cpp
GameEngine engine;

// Config setup before calling *engine.run*
EngineConfig config;
config.backgroundColor = RGBA(25, 196, 255, 255);
config.vsync = false;
// *any other values avalible in config*

// Creating a *Game* struct which is a custom implementation of *IGame*
Game game;

// Passing both to *engine.run* at the start, letting it handle the rest of the runtime
engine.Run(game, config);

return 0;
```

