#include <SDL2/SDL.h>
#include "Math/Vector.h"

class GameEngine
{
private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Event windowEvent;

    Uint64 FrameStartTime;

    void m_initVariables();
    void m_initWindow();
    void m_OnKeyboardInput(SDL_Event event);
public:
    static const int MAX_FRAME_RATE = 60;
    bool running = true;
    float deltaTime;    // time between frames in seconds
    float FPS;
    
    Vec2 mousePos;

    GameEngine();
    ~GameEngine();
    void StartFrame();
    void EndFrame();
    void Update();
    void PollEvents();
    void Render();
};