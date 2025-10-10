#include "Renderer.h"
#include <cmath>

Renderer::Renderer(int w, int h)
    : window(nullptr), renderer(nullptr), width(w), height(h), running(true) {
}

Renderer::~Renderer() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    window = SDL_CreateWindow("Lava Lamp Screensaver",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        return false;
    }

    return true;
}

void Renderer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT ||
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            running = false;
        }
    }
}

void Renderer::drawBackground(int h) {
    // Solid dark background; we keep it neutral and let the bottom light add warmth.
    SDL_SetRenderDrawColor(renderer, 8, 10, 14, 255);
    SDL_RenderClear(renderer);
}

void Renderer::drawMolecule(const Molecule& molecule) {
    // Color based on temperature (cool = blue, hot = red)
    float tempNorm = (molecule.temperature - 20.0f) / 60.0f;  // Normalize 20-80°C to 0-1
    tempNorm = std::max(0.0f, std::min(1.0f, tempNorm));

    int r = static_cast<int>(50 + tempNorm * 205);
    int g = static_cast<int>(150 - tempNorm * 100);
    int b = static_cast<int>(255 - tempNorm * 200);

    // Draw molecule as a filled circle (approximated with lines)
    SDL_SetRenderDrawColor(renderer, r, g, b, 220);

    int radius = static_cast<int>(molecule.radius);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderDrawPoint(renderer,
                                   static_cast<int>(molecule.position.x) + x,
                                   static_cast<int>(molecule.position.y) + y);
            }
        }
    }
}

void Renderer::drawLampLight() {
    // Simple vertical falloff from bottom to top.
    // We draw horizontal lines additively; brighter near the bottom.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    const float k = std::max(0.0f, std::min(1.0f, lightIntensity)); // clamp
    const int H = height;
    const int W = width;

    // Warm light color (bulb-like)
    const Uint8 baseR = 255, baseG = 210, baseB = 160;

    // Exponential falloff; tweak alpha by distance from the bottom
    for (int y = H - 1; y >= 0; --y) {
        float t = float(H - 1 - y) / float(H);       // 0 at bottom, 1 at top
        float falloff = std::exp(-4.5f * t);         // steeper near bottom
        float a = 32.0f * k * falloff;               // alpha contribution (0..~32)
        Uint8 alpha = (Uint8)std::min(255.0f, a);

        SDL_SetRenderDrawColor(renderer, baseR, baseG, baseB, alpha);
        SDL_RenderDrawLine(renderer, 0, y, W, y);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void Renderer::render(const LavaLamp& lamp) {
    drawBackground(lamp.getHeight());
    drawLampLight(); 

    for (const auto& mol : lamp.getMolecules()) {  
        drawMolecule(mol, lamp);                   
    }
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void Renderer::present() {
    SDL_RenderPresent(renderer);
}
