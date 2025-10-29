#include "Particle.h"

void Particle::update(float dt, int w, int h, float speed) {
    x += vx * speed * dt;
    y += vy * speed * dt;

    // rebote contra bordes
    if (x < 0.f)      { x = 0.f;      vx = -vx; } // izquierda
    if (x > w-1.f)    { x = w-1.f;    vx = -vx; } // derecha
    if (y < 0.f)      { y = 0.f;      vy = -vy; } // arriba
    if (y > h-1.f)    { y = h-1.f;    vy = -vy; } // abajo
}

// Aca es cuando activamos la rotación con R

void Particle::rotateAroundSC(float cx, float cy, float s, float c) {

    float rx = x - cx;
    float ry = y - cy;
    float nx = rx*c - ry*s;
    float ny = rx*s + ry*c;
    x = nx + cx;
    y = ny + cy;
}
