#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "LavaLamp.h"
#include "Palette.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    SDL_Window* getSDLWindow() const { return window; }
    int getWidth()  const { return width; }
    int getHeight() const { return height; }

    bool init();
    void render(const LavaLamp& lamp);
    void clear();
    void present();

    bool isRunning() const { return running; }
    void handleEvents();
    void update(float dt);

    void setLightIntensity(float k) { lightIntensity = k; }
    void setPalette(PaletteKind p)  { palette = p; }

private:
    SDL_Window*   window;
    SDL_Renderer* renderer;
    int width, height;
    bool running;

    float       lightIntensity = 0.6f;
    PaletteKind palette       = PaletteKind::RED;

    bool  bgWhite = false;
    bool  autoCycle = false;
    float autoCycleInterval = 1.0f;
    float autoCycleTimer = 0.0f;

    void drawBackground(int h);
    void drawLampLight();
    void drawMolecule(const Molecule& m, const LavaLamp& lamp);
    void nextPalette();
};

#endif
