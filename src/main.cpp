#include "LavaLamp.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    const int TARGET_FPS = 60;
    const float TARGET_FRAME_TIME = 1000.0f / TARGET_FPS;

    // Initialize renderer
    Renderer renderer(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!renderer.init()) {
        std::cerr << "Failed to initialize renderer: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create lava lamp simulation
    LavaLamp lamp(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Main loop
    Uint32 lastTime = SDL_GetTicks();
    while (renderer.isRunning()) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;  // Convert to seconds
        lastTime = currentTime;

        // Cap dt to avoid large jumps
        if (dt > 0.1f) dt = 0.1f;

        // Handle input
        renderer.handleEvents();

        // Update simulation
        lamp.update(dt);

        // Render
        renderer.clear();
        renderer.render(lamp);
        renderer.present();

        // Frame rate limiting
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < TARGET_FRAME_TIME) {
            SDL_Delay(static_cast<Uint32>(TARGET_FRAME_TIME - frameTime));
        }
    }

    return 0;
}
