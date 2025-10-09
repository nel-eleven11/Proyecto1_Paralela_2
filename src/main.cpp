#include "LavaLamp.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <cmath>

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

    //Variables for FPS 
    double fps_accum = 0.0;
    int    fps_frames = 0;
    double fps_smoothed = 0.0;

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

        // FPS calc every 1s
        fps_accum += dt;
        fps_frames++;

        if (fps_accum >= 1.0) {
            double fps_inst = fps_frames / fps_accum;
            
            fps_smoothed = (fps_smoothed == 0.0) ? fps_inst : (0.8 * fps_smoothed + 0.2 * fps_inst);

            // In the screen
            {
                std::ostringstream title;
                title << "Lava Lamp | " << renderer.getWidth() << "x" << renderer.getHeight()
                    << " | FPS=" << static_cast<int>(std::round(fps_smoothed));
                SDL_SetWindowTitle(/* SDL_Window* */ renderer.getSDLWindow(), title.str().c_str());
            }

            // In console
            std::cout << "FPS= " << fps_smoothed << std::endl;

            // Acumulators reset
            fps_accum = 0.0;
            fps_frames = 0;
        }


        // Frame rate limiting
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < TARGET_FRAME_TIME) {
            SDL_Delay(static_cast<Uint32>(TARGET_FRAME_TIME - frameTime));
        }
    }

    return 0;
}
