#include "App.h"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <algorithm>

#ifdef USE_OPENMP
  #include <omp.h>
#endif
static bool g_whiteBg = false;

App::~App() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool App::init(const Config& cfg) {
    cfg_ = cfg;
    radius2_ = cfg_.radius * cfg_.radius;
    cellSize_ = std::max(10.0f, cfg_.radius);

#ifdef USE_OPENMP
    if (cfg_.threads > 0) omp_set_num_threads(cfg_.threads);
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow("Screensaver OpenMP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg_.width, cfg_.height, SDL_WINDOW_SHOWN);
    if (!window_) {
        SDL_Log("CreateWindow error: %s", SDL_GetError());
        return false;
    }

    Uint32 flags = SDL_RENDERER_ACCELERATED | (cfg_.bench ? 0 : SDL_RENDERER_PRESENTVSYNC);
    renderer_ = SDL_CreateRenderer(window_, -1, flags);
    if (!renderer_) {
        SDL_Log("CreateRenderer error: %s", SDL_GetError());
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Partículas iniciales
    unsigned int seed = cfg_.seed ? cfg_.seed : (unsigned)SDL_GetTicks();
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> px(0.f, (float)cfg_.width);
    std::uniform_real_distribution<float> py(0.f, (float)cfg_.height);
    std::uniform_real_distribution<float> spd(-60.f, 60.f);
    std::uniform_int_distribution<int> col(160,255);

    particles_.resize(cfg_.n);
    for (int i=0;i<cfg_.n;i++) {
        particles_[i].x = px(rng);
        particles_[i].y = py(rng);
        particles_[i].vx = spd(rng);
        particles_[i].vy = spd(rng);
        particles_[i].r = (Uint8)col(rng);
        particles_[i].g = (Uint8)col(rng);
        particles_[i].b = (Uint8)col(rng);
    }

    // grid
    gw_ = std::max(1, (int)std::ceil(cfg_.width  / cellSize_));
    gh_ = std::max(1, (int)std::ceil(cfg_.height / cellSize_));
    cellCounts_.assign(gw_*gh_, 0);
    cellOffsets_.assign(gw_*gh_+1, 0);
    cellItems_.assign(cfg_.n, 0);

    setWindowTitle(0.f);
    return true;
}

void App::setWindowTitle(float fps) {
    std::ostringstream oss;
    oss << (cfg_.parallel ? "PAR" : "SEQ")
        << " | N="   << cfg_.n
        << " | r="   << (int)cfg_.radius
        << " | spd=" << cfg_.speed
        << " | C="   << (autoCycle_ ? "ON" : "OFF")
        << " | FPS=" << (int)fps
        << " | BG="  << (g_whiteBg ? "WHITE" : "BLACK");
    SDL_SetWindowTitle(window_, oss.str().c_str());
}


// Eventos dentro del screen saver
void App::handleEvents(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_SPACE:  paused_ = !paused_; break;

                case SDLK_r:
                    rotationSign_ = (rotationSign_==0) ? +1 : (rotationSign_==+1 ? -1 : 0);
                    break;

                case SDLK_F1: palette_ = Palette::Neon;   break;
                case SDLK_F2: palette_ = Palette::Sunset; break;
                case SDLK_F3: palette_ = Palette::Aqua;   break;
                case SDLK_F4: palette_ = Palette::Candy;  break;

                case SDLK_c:  autoCycle_ = !autoCycle_; cycleTimer_ = 0.f; break;

                case SDLK_LEFT:
                    cfg_.speed = std::max(0.1f, cfg_.speed - 0.1f);
                    break;
                case SDLK_RIGHT:
                    cfg_.speed = std::min(5.0f,  cfg_.speed + 0.1f);
                    break;

                case SDLK_UP:
                    cfg_.radius += 5.f; radius2_ = cfg_.radius * cfg_.radius; break;
                case SDLK_DOWN:
                    cfg_.radius -= 5.f; if (cfg_.radius < 10.f) cfg_.radius = 10.f; radius2_ = cfg_.radius * cfg_.radius; break;

                case SDLK_b:
                    g_whiteBg = !g_whiteBg;
                    break;

                default: break;
            }
        }
    }
}

// version sequencial

void App::rebuildGridSequential() { // se arma el grid

    std::fill(cellCounts_.begin(), cellCounts_.end(), 0);

    // numero de particulas por celda
    for (int i=0;i<cfg_.n;i++) {
        int cx = std::clamp((int)(particles_[i].x / cellSize_), 0, gw_-1);
        int cy = std::clamp((int)(particles_[i].y / cellSize_), 0, gh_-1);
        cellCounts_[cellId(cx,cy)]++;
    }

    // calcular donde inicia la lista de indices de cada celda
    cellOffsets_[0] = 0;
    for (int c=0;c<gw_*gh_; ++c) cellOffsets_[c+1] = cellOffsets_[c] + cellCounts_[c];

    // escribimos índices de partículas por celda
    std::vector<int> curs = cellCounts_;
    for (int i=0;i<cfg_.n;i++) {
        int cx = std::clamp((int)(particles_[i].x / cellSize_), 0, gw_-1);
        int cy = std::clamp((int)(particles_[i].y / cellSize_), 0, gh_-1);
        int id = cellId(cx,cy);
        int pos = cellOffsets_[id] + (--curs[id]);
        cellItems_[pos] = i;
    }
}

// versión sequencial
void App::buildEdgesSeq(float dt) {
    const int   winW   = cfg_.width, winH = cfg_.height;
    const float r2     = radius2_;

    // Por si usamos R nada mas
    const float s      = (rotationSign_ ? std::sin(rotationSign_ * rotationSpeed_ * dt) : 0.f);
    const float c      = (rotationSign_ ? std::cos(rotationSign_ * rotationSpeed_ * dt) : 1.f);

    for (auto& p : particles_) {
        p.update(dt, winW, winH, cfg_.speed);
        if (rotationSign_) p.rotateAroundSC(winW*0.5f, winH*0.5f, s, c);
    }

    rebuildGridSequential();

    // limpieza inicial
    edges_.clear();
    edges_.reserve((size_t)cfg_.n * 8);

    // revisar cada celda con ella misma y con 4 vecinas (derecha, abajo-der, abajo, abajo-izq)
    const int OFFSETX[5] = {0, 1, 1, 0, -1};
    const int OFFSETY[5] = {0, 0, 1, 1,  1};

    const int totalCells = gw_ * gh_;
    for (int cellIdFlat = 0; cellIdFlat < totalCells; ++cellIdFlat) {
        // coordenadas (x,y) de la celda actual
        const int cellX = cellIdFlat % gw_;
        const int cellY = cellIdFlat / gw_;

        const int cellStart = cellOffsets_[cellIdFlat];
        const int cellEnd   = cellOffsets_[cellIdFlat + 1];

        // Aca recorre los OFFSET
        for (int k = 0; k < 5; ++k) {
            const int neighborX = cellX + OFFSETX[k];
            const int neighborY = cellY + OFFSETY[k];

            // validación de bordes
            if (neighborX < 0 || neighborX >= gw_ || neighborY < 0 || neighborY >= gh_) continue;

            const int neighborId    = neighborY * gw_ + neighborX;
            const int neighborStart = cellOffsets_[neighborId];
            const int neighborEnd   = cellOffsets_[neighborId + 1];

            if (k == 0) {
                for (int idxInCurrentA = cellStart; idxInCurrentA < cellEnd; ++idxInCurrentA) {
                    for (int idxInCurrentB = idxInCurrentA + 1; idxInCurrentB < cellEnd; ++idxInCurrentB) {
                        const int particleAIdx = cellItems_[idxInCurrentA];
                        const int particleBIdx = cellItems_[idxInCurrentB];

                        const float dx = particles_[particleAIdx].x - particles_[particleBIdx].x;
                        const float dy = particles_[particleAIdx].y - particles_[particleBIdx].y;
                        const float d2 = dx*dx + dy*dy;

                        if (d2 <= r2) {
                            edges_.push_back({ particleAIdx, particleBIdx, 1.f - d2/r2 }); // se crea la arista con la intensidad
                        }
                    }
                }
            } else {
                // celdas vecinas todos contra todos
                for (int idxInCurrent = cellStart; idxInCurrent < cellEnd; ++idxInCurrent) {
                    const int particleAIdx = cellItems_[idxInCurrent];
                    for (int idxInNeighbor = neighborStart; idxInNeighbor < neighborEnd; ++idxInNeighbor) {
                        const int particleBIdx = cellItems_[idxInNeighbor];
                        const float dx = particles_[particleAIdx].x - particles_[particleBIdx].x;
                        const float dy = particles_[particleAIdx].y - particles_[particleBIdx].y;
                        const float d2 = dx*dx + dy*dy; // distancia^2
                        if (d2 <= r2) {
                            edges_.push_back({ particleAIdx, particleBIdx, 1.f - d2/r2 });
                        }
                    }
                }
            }
        }
    }
}

// versión paralela
void App::buildEdgesPar(float dt) {


    const int   winW = cfg_.width, winH = cfg_.height;
    const float r2   = radius2_;

    // Solo si usa R
    const float s    = (rotationSign_ ? std::sin(rotationSign_ * rotationSpeed_ * dt) : 0.f);
    const float c    = (rotationSign_ ? std::cos(rotationSign_ * rotationSpeed_ * dt) : 1.f);

    for (auto& p : particles_) {
        p.update(dt, winW, winH, cfg_.speed);
        if (rotationSign_) p.rotateAroundSC(winW*0.5f, winH*0.5f, s, c);
    }

    rebuildGridSequential();

    const int OFFSETX[5] = {0, 1, 1, 0, -1}; // misma, der, ab-der, ab, ab-izq
    const int OFFSETY[5] = {0, 0, 1, 1,  1};

    const int totalCells = gw_ * gh_;

    // aca empezamos a paralelizar
    std::vector<std::vector<Edge>> perThread;
    int numThreads = 1;

    #pragma omp parallel
    {
        // descubrimos cuántos hilos hay
        #pragma omp single
        {
            numThreads = omp_get_num_threads();
            perThread.resize(std::max(1, numThreads));
        }

        // cada hilo trabaja en su vector local
        int tid = 0;
        #ifdef _OPENMP
            tid = omp_get_thread_num();
        #endif
        auto& localEdges = perThread[tid];
        localEdges.clear();
        localEdges.reserve((size_t)cfg_.n * 4 / std::max(1, numThreads) + 256);

        // repartimos las celdas; guided ayuda cuando algunas celdas tienen más partículas que otras
        #pragma omp for schedule(guided)
        for (int cellIdFlat = 0; cellIdFlat < totalCells; ++cellIdFlat) {

            // coordenadas (x,y) de la celda actual
            const int cellX = cellIdFlat % gw_;
            const int cellY = cellIdFlat / gw_;

            // rango [cellStart, cellEnd) dentro de cellItems_ para esta celda
            const int cellStart = cellOffsets_[cellIdFlat];
            const int cellEnd   = cellOffsets_[cellIdFlat + 1];

            // revisar 5 destinos: misma celda y 4 vecinas (sin duplicar)
            for (int k = 0; k < 5; ++k) {
                const int neighborX = cellX + OFFSETX[k];
                const int neighborY = cellY + OFFSETY[k];

                // si la vecina se sale, la saltamos
                if (neighborX < 0 || neighborX >= gw_ || neighborY < 0 || neighborY >= gh_) continue;

                const int neighborId    = neighborY * gw_ + neighborX;
                const int neighborStart = cellOffsets_[neighborId];
                const int neighborEnd   = cellOffsets_[neighborId + 1];

                if (k == 0) {
                    // MISMA celda: combinaciones sin repetir (B empieza después de A)
                    for (int idxInCurrentA = cellStart; idxInCurrentA < cellEnd; ++idxInCurrentA) {
                        for (int idxInCurrentB = idxInCurrentA + 1; idxInCurrentB < cellEnd; ++idxInCurrentB) {
                            const int particleAIdx = cellItems_[idxInCurrentA]; // índice real en particles_
                            const int particleBIdx = cellItems_[idxInCurrentB];

                            const float dx = particles_[particleAIdx].x - particles_[particleBIdx].x;
                            const float dy = particles_[particleAIdx].y - particles_[particleBIdx].y;
                            const float d2 = dx*dx + dy*dy; // distancia^2

                            if (d2 <= r2) {
                                const float weight = 1.f - d2/r2; // más cerca => más intenso
                                localEdges.push_back({ particleAIdx, particleBIdx, weight });
                            }
                        }
                    }
                } else {
                    // CELDA vecina: todos contra todos entre actual y vecina
                    for (int idxInCurrent = cellStart; idxInCurrent < cellEnd; ++idxInCurrent) {
                        const int particleAIdx = cellItems_[idxInCurrent];
                        for (int idxInNeighbor = neighborStart; idxInNeighbor < neighborEnd; ++idxInNeighbor) {
                            const int particleBIdx = cellItems_[idxInNeighbor];

                            const float dx = particles_[particleAIdx].x - particles_[particleBIdx].x;
                            const float dy = particles_[particleAIdx].y - particles_[particleBIdx].y;
                            const float d2 = dx*dx + dy*dy; // distancia 

                            if (d2 <= r2) {
                                const float weight = 1.f - d2/r2;
                                localEdges.push_back({ particleAIdx, particleBIdx, weight });
                            }
                        }
                    }
                }
            }
        }
    }


    // se mergean todos los hilos
    size_t totalEdges = 0;
    for (auto& v : perThread) totalEdges += v.size();

    edges_.clear();
    edges_.reserve(totalEdges);
    for (auto& v : perThread) {
        edges_.insert(edges_.end(), v.begin(), v.end());
    }
}



void App::update(float dt) {

    if (autoCycle_) {
        cycleTimer_ += dt;
        if (cycleTimer_ >= cycleEvery_) {
            cycleTimer_ = 0.f;
            int p = static_cast<int>(palette_);
            p = (p + 1) % 4;
            palette_ = static_cast<Palette>(p);
        }
    }
    globalAngle_ += rotationSign_ * rotationSpeed_ * dt;
    if (globalAngle_ > 6.28318f)  globalAngle_ -= 6.28318f;
    if (globalAngle_ < -6.28318f) globalAngle_ += 6.28318f;

    if (cfg_.parallel) buildEdgesSeq(dt);
    else               buildEdgesPar(dt);

    setWindowTitle(timer_.fps());
}

void App::render() {
    if (cfg_.bench) return;

    if (g_whiteBg) SDL_SetRenderDrawColor(renderer_, 245, 245, 247, 255);
    else           SDL_SetRenderDrawColor(renderer_,  10,  10,  12, 255);
    SDL_RenderClear(renderer_);

    // líneas: color/alpha en función de la cercanía
    for (const auto& e : edges_) {
        const auto& a = particles_[e.a];
        const auto& b = particles_[e.b];

        SDL_Color c = paletteColor(palette_, e.w);
        Uint8 alpha = static_cast<Uint8>(40 + 200 * e.w);
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, alpha);
        SDL_RenderDrawLine(renderer_, (int)a.x, (int)a.y, (int)b.x, (int)b.y);
    }

    for (const auto& p : particles_) {
        SDL_SetRenderDrawColor(renderer_, p.r, p.g, p.b, 220);
        SDL_Rect r{ (int)p.x-1, (int)p.y-1, 3, 3 };
        SDL_RenderFillRect(renderer_, &r);
    }

    SDL_RenderPresent(renderer_);
}

void App::run() {
    bool running = true;
    while (running) {
        handleEvents(running);
        float dt = timer_.tick();
        if (!paused_) update(dt);
        render();
    }
}
