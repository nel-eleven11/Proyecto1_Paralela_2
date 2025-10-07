#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "LavaLamp.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    bool init();
    void render(const LavaLamp& lamp);
    void clear();
    void present();

    bool isRunning() const { return running; }
    void handleEvents();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height;
    bool running;

    void drawBlob(const Blob& blob);
    void drawBackground(int height);
};

#endif
