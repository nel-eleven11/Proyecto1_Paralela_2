#include "LavaLamp.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <string>
#include <getopt.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    const int TARGET_FPS = 60;
    const float TARGET_FRAME_TIME = 1000.0f / TARGET_FPS;

    int   numMolecules = 20;
    int   windowWidth  = 800;
    int   windowHeight = 600;
    float lightK       = 0.6f;
    std::string paletteName = "red";

    int opt;
    while ((opt = getopt(argc, argv, "n:w:h:L:p:")) != -1) {
        switch(opt) {
            case 'n': numMolecules = std::max(1, std::stoi(optarg)); break;
            case 'w': windowWidth  = std::max(640, std::stoi(optarg)); break;
            case 'h': windowHeight = std::max(480, std::stoi(optarg)); break;
            case 'L': lightK = std::stof(optarg); break;
            case 'p': paletteName = optarg; break;
            default:
                std::cerr << "Usage: " << argv[0]
                          << " [-n molecules] [-w width>=640] [-h height>=480]"
                          << " [-L light_0_1] [-p palette]\n";
                return 1;
        }
    }

    Renderer renderer(windowWidth, windowHeight);
    if (!renderer.init()) {
        std::cerr << "Failed to initialize renderer: " << SDL_GetError() << std::endl;
        return 1;
    }
    renderer.setLightIntensity(std::max(0.0f, std::min(1.0f, lightK)));
    renderer.setPalette(parsePalette(paletteName));

    LavaLamp lamp(windowWidth, windowHeight, numMolecules);

    double fps_accum = 0.0;
    int    fps_frames = 0;
    double fps_smoothed = 0.0;

    Uint32 lastTime = SDL_GetTicks();
    while (renderer.isRunning()) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;

        renderer.handleEvents();
        renderer.update(dt);

        lamp.update(dt);

        renderer.clear();
        renderer.render(lamp);
        renderer.present();

        fps_accum += dt;
        fps_frames++;
        if (fps_accum >= 1.0) {
            double fps_inst = fps_frames / fps_accum;
            fps_smoothed = (fps_smoothed == 0.0) ? fps_inst : (0.8 * fps_smoothed + 0.2 * fps_inst);
            std::ostringstream title;
            title << "Lava Lamp | " << renderer.getWidth() << "x" << renderer.getHeight()
                  << " | FPS=" << static_cast<int>(std::round(fps_smoothed));
            SDL_SetWindowTitle(renderer.getSDLWindow(), title.str().c_str());
            fps_accum = 0.0;
            fps_frames = 0;
        }

        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < TARGET_FRAME_TIME) {
            SDL_Delay(static_cast<Uint32>(TARGET_FRAME_TIME - frameTime));
        }
    }

    return 0;
}
