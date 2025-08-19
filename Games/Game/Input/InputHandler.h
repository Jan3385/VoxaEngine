#pragma once

#include "Input/MouseDataHandler.h"

namespace Input{
    extern Input::MouseData mouseData;

    void OnMouseScroll(int yOffset);
    void OnMouseButtonDown(int button);
    void OnMouseButtonUp(int button);
    void OnMouseMove(int x, int y);
    void OnKeyboardDown(int key);
    void OnKeyboardUp(int key);
}