#include "LavaLamp.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <getopt.h>

int main(int argc, char* argv[]) {
    // Constants and default variables
    const int TARGET_FPS = 60;
    const float TARGET_FRAME_TIME = 1000.0f / TARGET_FPS;
    int numMolecules = 20;
    int windowWidth = 800;  
    int windowHeight = 600; 
    float lightK = 0.6f;         // default lamp light intensity
    std::string paletteName = "red";

    // Process command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:w:h:L:p:")) != -1) {
    switch(opt) {
        case 'n': numBlobs = std::stoi(optarg); break;
        case 'w': windowWidth  = std::max(640, std::stoi(optarg)); break;
        case 'h': windowHeight = std::max(480, std::stoi(optarg)); break;
        case 'L': lightK = std::stof(optarg); break;      // 0..1 suggested
        case 'p': paletteName = optarg; break;            // red, blue, orange, green, purple, rainbow
        default:
            std::cerr << "Usage: " << argv[0]
                      << " [-n number_of_molecules] [-w width] [-h height]"
                      << " [-L light_intensity_0_1] [-p palette]\n";
            return 1;
    }
}


    // Initialize renderer
    Renderer renderer(windowWidth, windowHeight);
    if (!renderer.init()) {
        std::cerr << "Failed to initialize renderer: " << SDL_GetError() << std::endl;
        return 1;
    }

    renderer.setLightIntensity(std::max(0.0f, std::min(1.0f, lightK)));
    renderer.setPalette(parsePalette(paletteName));

    // Create lava lamp simulation
    LavaLamp lamp(windowWidth, windowHeight, numMolecules);

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
