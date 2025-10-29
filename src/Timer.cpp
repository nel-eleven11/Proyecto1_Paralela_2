#include "Timer.h"

Timer::Timer() {
    prev_ = SDL_GetPerformanceCounter();
    acc_ = 0.f;
    frames_ = 0;
    lastFPS_ = 0.f;
}

float Timer::tick() {
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)((now - prev_) / (double)SDL_GetPerformanceFrequency());
    prev_ = now;

    acc_ += dt;
    frames_++;
    if (acc_ >= 0.5f) {
        lastFPS_ = frames_ / acc_;
        frames_ = 0;
        acc_ = 0.f;
    }
    return dt;
}

float Timer::fps() const { return lastFPS_; }
