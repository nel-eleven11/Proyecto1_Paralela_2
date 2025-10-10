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
    // Draw gradient background (dark blue at top, orange at bottom)
    for (int y = 0; y < h; y++) {
        float t = static_cast<float>(y) / h;

        // Top: dark blue (20, 20, 60)
        // Bottom: orange (200, 80, 20)
        int r = static_cast<int>(20 + t * 180);
        int g = static_cast<int>(20 + t * 60);
        int b = static_cast<int>(60 - t * 40);

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer, 0, y, width, y);
    }
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

void Renderer::render(const LavaLamp& lamp) {
    drawBackground(lamp.getHeight());

    for (const auto& molecule : lamp.getMolecules()) {
        drawMolecule(molecule);
    }
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void Renderer::present() {
    SDL_RenderPresent(renderer);
}
