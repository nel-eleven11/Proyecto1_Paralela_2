#pragma once
#include <SDL.h>
#include <vector>
#include "Args.h"
#include "Particle.h"
#include "Color.h"
#include "Timer.h"

struct Edge { int a; int b; float w; };

class App {
public:
    App() = default;
    ~App();

    bool init(const Config& cfg);
    void run();

private:
    void handleEvents(bool& running);
    void update(float dt);

    // versión secuencial
    void rebuildGridSequential();
    void buildEdgesSeq(float dt);

    // versión paralela
#ifdef USE_OPENMP
    void rebuildGridParallel(int maxThreads);
#endif
    void buildEdgesPar(float dt);

    void render();
    void setWindowTitle(float fps);

    inline int cellId(int cx, int cy) const { return cy*gw_ + cx; }

private:
    Config cfg_{};
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    std::vector<Particle> particles_;
    std::vector<Edge> edges_;

    // grid plano
    int gw_ = 1, gh_ = 1;
    float cellSize_ = 80.f;
    std::vector<int>   cellCounts_;
    std::vector<int>   cellOffsets_;
    std::vector<int>   cellItems_;
#ifdef USE_OPENMP
    std::vector<int> particleCellIds_;
    std::vector<int> perThreadCounts_;
    std::vector<int> perThreadOffsets_;
#endif

    Timer timer_;

    bool  paused_        = false;
    int   rotationSign_  = 0;     
    float rotationSpeed_ = 1.6f;  
    float globalAngle_   = 0.0f;

    // color/paletas
    Palette palette_  = Palette::Neon;
    bool    autoCycle_   = false;
    float   cycleEvery_  = 2.0f;
    float   cycleTimer_  = 0.0f;

    float radius2_     = 0.0f;
    float invRadius2_  = 0.0f;
};

