#include "Renderer.h"
#include <cmath>
#include <algorithm>

Renderer::Renderer(int w, int h)
    : window(nullptr), renderer(nullptr), width(w), height(h), running(true) {}

Renderer::~Renderer() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    window = SDL_CreateWindow("Lava Lamp Screensaver",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    return true;
}

void Renderer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT ||
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            running = false;
        } else if (event.type == SDL_KEYDOWN) {
            SDL_Keycode k = event.key.keysym.sym;
            if (k == SDLK_c) nextPalette();
            else if (k == SDLK_b) bgWhite = !bgWhite;
            else if (k == SDLK_a) autoCycle = !autoCycle, autoCycleTimer = 0.0f;
            else if (k == SDLK_q) autoCycleInterval = std::max(0.1f, autoCycleInterval - 0.1f);
            else if (k == SDLK_e) autoCycleInterval = std::min(5.0f, autoCycleInterval + 0.1f);
        }
    }
}

void Renderer::update(float dt) {
    if (!autoCycle) return;
    autoCycleTimer += dt;
    if (autoCycleTimer >= autoCycleInterval) {
        nextPalette();
        autoCycleTimer = 0.0f;
    }
}

void Renderer::drawBackground(int) {
    if (bgWhite) SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    else         SDL_SetRenderDrawColor(renderer, 8, 10, 14, 255);
    SDL_RenderClear(renderer);
}

void Renderer::drawLampLight() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    const float k = std::clamp(lightIntensity, 0.0f, 1.0f);
    const int H = height;
    const int W = width;
    const Uint8 baseR = 255, baseG = 210, baseB = 160;
    for (int y = H - 1; y >= 0; --y) {
        float t = float(H - 1 - y) / float(H);
        float falloff = std::exp(-4.5f * t);
        float a = 32.0f * k * falloff;
        Uint8 alpha = (Uint8)std::min(255.0f, a);
        SDL_SetRenderDrawColor(renderer, baseR, baseG, baseB, alpha);
        SDL_RenderDrawLine(renderer, 0, y, W, y);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void Renderer::drawBlobFill(const LavaLamp& lamp) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Slightly lighter than molecule colors: mix with white (weight ~0.35)
    auto lighten = [](RGB c)->RGB{
        auto L = [](uint8_t a, uint8_t b, float t){
            return (uint8_t)std::round((1.0f - t) * a + t * b);
        };
        return { L(c.r, 255, 0.35f), L(c.g, 255, 0.35f), L(c.b, 255, 0.35f) };
    };

    const auto& mols  = lamp.getMolecules();
    const auto& blobs = lamp.getBlobs();

    for (const auto& kv : blobs) {
        const Blob& B = kv.second;
        if (B.members.size() < 2) continue; // need at least 2 to show fill

        // Base color from palette using blob center coordinate as seed
        float u = std::fmod(std::abs(B.center.x * 0.005f + B.center.y * 0.003f), 1.0f);
        RGB base = pickFromPalette(palette, u);
        RGB col  = lighten(base);

        // Slight alpha for soft union of expanded discs
        Uint8 A = 120;

        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, A);

        for (auto idx : B.members) {
            const Molecule& m = mols[idx];

            // Expanded radius = radius + 50% of padding so that visible fill appears
            float R = std::max(1.0f, m.radius + 0.4f * m.padding);
            int   ri = (int)std::round(R);

            int cx = (int)std::round(m.position.x);
            int cy = (int)std::round(m.position.y);

            // draw filled circle 
            for (int y = -ri; y <= ri; ++y) {
                int span = (int)std::floor(std::sqrt((double)ri*ri - (double)y*y));
                int py = cy + y;
                if (py < 0 || py >= height) continue;
                int x0 = std::max(0, cx - span);
                int x1 = std::min(width - 1, cx + span);
                SDL_RenderDrawLine(renderer, x0, py, x1, py);
            }
        }
    }
}

void Renderer::drawMolecule(const Molecule& m, const LavaLamp& lamp) {
    const auto& blobs = lamp.getBlobs();
    RGB baseColor{200,60,60};
    Vec2 center = m.position;
    float blobR = m.radius;

    auto it = blobs.find(m.blobId);
    if (it != blobs.end()) {
        center = it->second.center;
        blobR  = std::max(m.radius, it->second.approxRadius);
    }

    float dx = m.position.x - center.x;
    float dy = m.position.y - center.y;
    float d  = std::sqrt(dx*dx + dy*dy);
    float R  = std::max(1.0f, blobR);

    float core = std::exp(-2.5f * (d/R) * (d/R));
    float rim = 0.0f;
    if (d > 0.7f*R) {
        float t = (d - 0.7f*R) / (0.35f*R + 1e-3f);
        rim = std::exp(-8.0f * t*t);
    }

    float u = std::fmod(std::abs(m.position.x*0.013f + m.position.y*0.007f), 1.0f);
    baseColor = pickFromPalette(palette, u);

    float ambientAtPoint = lamp.getTemperatureAt(m.position.x, m.position.y);
    float tempDiff = m.temperature - ambientAtPoint;
    float heatFactor = std::clamp((tempDiff + 35.0f) / 70.0f, 0.0f, 1.6f);

    float brightness = std::min(1.0f, 0.28f + 0.70f*core + 0.30f*rim);
    brightness *= (0.65f + 1.0f * heatFactor);

    Uint8 r = (Uint8)std::min(255.0f, baseColor.r * brightness);
    Uint8 g = (Uint8)std::min(255.0f, baseColor.g * brightness);
    Uint8 b = (Uint8)std::min(255.0f, baseColor.b * brightness);

    SDL_SetRenderDrawColor(renderer, r, g, b, 240);

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

void Renderer::render(const LavaLamp& lamp) {
    drawBackground(lamp.getHeight());
    drawLampLight();
    drawBlobFill(lamp);
    for (const auto& mol : lamp.getMolecules()) drawMolecule(mol, lamp);
}

void Renderer::clear() {
    if (bgWhite) SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    else         SDL_SetRenderDrawColor(renderer, 8, 10, 14, 255);
    SDL_RenderClear(renderer);
}

void Renderer::present() { SDL_RenderPresent(renderer); }

void Renderer::nextPalette() {
    switch (palette) {
        case PaletteKind::RED:     palette = PaletteKind::ORANGE; break;
        case PaletteKind::ORANGE:  palette = PaletteKind::GREEN;  break;
        case PaletteKind::GREEN:   palette = PaletteKind::BLUE;   break;
        case PaletteKind::BLUE:    palette = PaletteKind::PURPLE; break;
        case PaletteKind::PURPLE:  palette = PaletteKind::RAINBOW;break;
        case PaletteKind::RAINBOW: palette = PaletteKind::RED;    break;
    }
}
