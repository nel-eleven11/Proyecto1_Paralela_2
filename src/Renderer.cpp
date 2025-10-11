#include "Renderer.h"
#include <cmath>
#include "Palette.h"


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

void Renderer::drawMolecule(const Molecule& m, const LavaLamp& lamp) {
    // Fetch its blob (cluster) to get centroid and approximate radius
    const auto& blobs = lamp.getBlobs();
    RGB baseColor{200,60,60};
    float centerBoost = 1.0f; // intensity multiplier at core

    Vec2 center = m.position;
    float blobR = m.radius;

    auto it = blobs.find(m.blobId);
    if (it != blobs.end()) {
        center = it->second.center;
        blobR  = std::max(m.radius, it->second.approxRadius);
    }

    // Distance to blob center
    float dx = m.position.x - center.x;
    float dy = m.position.y - center.y;
    float d  = std::sqrt(dx*dx + dy*dy);
    float R  = std::max(1.0f, blobR);

    // Core intensity (stronger in the center)
    float core = std::exp(-2.5f * (d/R) * (d/R)); // 1 at center, fades outward

    // Rim highlight (thin bright edge near boundary)
    float rim = 0.0f;
    if (d > 0.7f*R) {
        float t = (d - 0.7f*R) / (0.35f*R + 1e-3f);
        rim = std::exp(-8.0f * t*t); // quick highlight near rim
    }

    // Pseudo-random pick in palette (stable per molecule)
    float u = std::fmod( std::abs( (m.position.x*0.013f) + (m.position.y*0.007f) ), 1.0f );
    baseColor = pickFromPalette(palette, u);

    // Compose color with core brightness + rim highlight
    float brightness = std::min(1.0f, 0.25f + 0.75f*core + 0.35f*rim);

    Uint8 r = (Uint8)std::min(255.0f, baseColor.r * brightness);
    Uint8 g = (Uint8)std::min(255.0f, baseColor.g * brightness);
    Uint8 b = (Uint8)std::min(255.0f, baseColor.b * brightness);

    SDL_SetRenderDrawColor(renderer, r, g, b, 230);

    int radius = (int)std::max(1.0f, m.radius);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderDrawPoint(renderer,
                    (int)std::round(m.position.x) + x,
                    (int)std::round(m.position.y) + y);
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
