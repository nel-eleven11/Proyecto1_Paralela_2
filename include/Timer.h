#pragma once
#include <SDL.h>

class Timer {
public:
    Timer();
    float tick();
    float fps() const;
private:
    Uint64 prev_;
    float acc_;
    int frames_;
    float lastFPS_;
};
