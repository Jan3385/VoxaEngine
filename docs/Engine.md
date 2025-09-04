### Engine

The engine is the main class handling all relevant data and programs runtime.
Your programs runtime should be only handled via the `IGame` struct implementation passed on during the `GameEngine::Run` method which should be only called once at the start of the program.

The engine is configured by the `EngineConfig` struct, which is also passed during the `GameEngine::Run` call

Minimal example for the engine startup inside `main` 

```cpp
GameEngine engine;

// Config setup before calling *engine.run*
EngineConfig config;
config.backgroundColor = RGB(255, 0, 255);
config.vsync = true;
// *any other values avalible in config*

// Creating a *Game* struct which is a custom implementation of *IGame*
Game game;

// Passing both to *engine.run* at the start, letting it handle the rest of the runtime
engine.Run(game, config);

return 0;
```

## Player

The player variable inside the GameEngine does **not** have to be set for the game to function properly. Its only usage is during the rendering process, where it is rendered on top of all other objects. *Any player logic is for the user to handle*

## Camera

Camera movement is handeled by the user. The final camera position should be given to the Renderer inside the `GameEngine` using `GameRenderer::SetCameraPosition`. Doing so can load all the chunks in the screen view based on the config `EngineConfig::automaticLoadingOfChunksInView`. If the config is set to false, the chunks in player's view won't load

## DeltaTimes

The `GameEngine::fixedDeltaTime` and `GameEngine::voxelFixedDeltaTime` *can* be modified at any time during the programs lifetime safely but that should be avoided if possible. Changing those values impacts both the Engine's build in fixed & voxel updates but also the ones in `IGame`. These values are also inside the `EngineConfig`, which sets them before running anything

`IGame::Update` provides a standard `deltaTime` variable which works as any standard delta time as it is the time between frames in seconds.

> [!NOTE]
> The deltaTime is the time between the previous last two frames. This is a technical issue almost all engines have. This can be ignored in most if not all scenarios

## Misc

`GameEngine::MovementKeysHeld[4]` is a bool array to easily access held movement keys in the following order: [W, S, A, D]

`GameEngine::running` can be set to `false` to force a proper shutdown after the game finishes executing the game loop

`GameEngine::runHeatSimulation`, `GameEngine::runPressureSimulation` & `GameEngine::runChemicalReactions` are bools which can be used to stop and start the GPU ran simulations at any time. Doing so will improve performance slightly.
