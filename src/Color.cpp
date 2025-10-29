#include "Color.h"
#include <algorithm>

SDL_Color lerp(const SDL_Color& a, const SDL_Color& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    SDL_Color c;
    c.r = static_cast<Uint8>(a.r + (b.r - a.r) * t);
    c.g = static_cast<Uint8>(a.g + (b.g - a.g) * t);
    c.b = static_cast<Uint8>(a.b + (b.b - a.b) * t);
    c.a = 255;
    return c;
}


SDL_Color paletteColor(Palette p, float t) {
    SDL_Color c0, c1, c2;
    switch (p) {
        case Palette::Neon:   c0={  0,255,200,255}; c1={  0,180,255,255}; c2={120,255, 80,255}; break;
        case Palette::Sunset: c0={255,120, 80,255}; c1={255, 60,120,255}; c2={255,210,110,255}; break;
        case Palette::Aqua:   c0={ 40,200,255,255}; c1={  0,255,150,255}; c2={  0,110,255,255}; break;
        case Palette::Candy:  c0={255, 80,180,255}; c1={150, 80,255,255}; c2={255,180, 80,255}; break;
    }
    if (t < 0.5f) return lerp(c0, c1, t*2.f); // t es el tramo para colorear
    return lerp(c1, c2, (t-0.5f)*2.f);
}
