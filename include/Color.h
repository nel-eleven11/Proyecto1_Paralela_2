#pragma once
#include <SDL.h>
#include <cstdint>

enum class Palette : uint8_t { Neon=0, Sunset=1, Aqua=2, Candy=3 };

SDL_Color lerp(const SDL_Color& a, const SDL_Color& b, float t);
SDL_Color paletteColor(Palette p, float t);
