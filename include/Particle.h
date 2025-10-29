#pragma once
#include <SDL.h>

struct Particle {
    float x, y;   // posición
    float vx, vy; // velocidad
    Uint8  r, g, b; // color base

    void update(float dt, int w, int h, float speed);
    // Versión que usa seno/coseno ya calculados (más barata que llamar sin/cos por partícula)
    void rotateAroundSC(float cx, float cy, float s, float c);
};
