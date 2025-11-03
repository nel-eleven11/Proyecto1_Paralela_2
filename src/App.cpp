#include "App.h"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iostream>

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
    radius2_    = cfg_.radius * cfg_.radius;
    invRadius2_ = (radius2_ > 0.0f ? 1.0f / radius2_ : 0.0f);
    cellSize_   = std::max(10.0f, cfg_.radius);

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

    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (!cfg_.bench && !cfg_.novsync) {
        flags |= SDL_RENDERER_PRESENTVSYNC;
    }
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

    // Print FPS to stdout for testing/benchmarking (every 30 frames)
    static int frameCount = 0;
    if (++frameCount >= 30) {
        std::cout << "FPS=" << (int)fps << std::endl;
        frameCount = 0;
    }
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
                    cfg_.radius += 5.f;
                    radius2_    = cfg_.radius * cfg_.radius;
                    invRadius2_ = (radius2_ > 0.0f ? 1.0f / radius2_ : 0.0f);
                    break;

                case SDLK_DOWN:
                    cfg_.radius -= 5.f;
                    if (cfg_.radius < 10.f) cfg_.radius = 10.f;
                    radius2_    = cfg_.radius * cfg_.radius;
                    invRadius2_ = (radius2_ > 0.0f ? 1.0f / radius2_ : 0.0f);
                    break;

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

    const int totalCells = gw_ * gh_;

    // número de partículas por celda
    for (int i = 0; i < cfg_.n; ++i) {
        int cx = static_cast<int>(particles_[i].x / cellSize_);
        if (cx < 0) cx = 0;
        else if (cx >= gw_) cx = gw_ - 1;

        int cy = static_cast<int>(particles_[i].y / cellSize_);
        if (cy < 0) cy = 0;
        else if (cy >= gh_) cy = gh_ - 1;

        cellCounts_[cellId(cx, cy)]++;
    }

    // offsets de inicio de cada celda
    cellOffsets_[0] = 0;
    for (int c = 0; c < totalCells; ++c) {
        cellOffsets_[c + 1] = cellOffsets_[c] + cellCounts_[c];
    }

    // escribimos índices de partículas por celda
    std::vector<int> curs = cellCounts_;
    for (int i = 0; i < cfg_.n; ++i) {
        int cx = static_cast<int>(particles_[i].x / cellSize_);
        if (cx < 0) cx = 0;
        else if (cx >= gw_) cx = gw_ - 1;

        int cy = static_cast<int>(particles_[i].y / cellSize_);
        if (cy < 0) cy = 0;
        else if (cy >= gh_) cy = gh_ - 1;

        int id  = cellId(cx, cy);
        int pos = cellOffsets_[id] + (--curs[id]);
        cellItems_[pos] = i;
    }
}

#ifdef USE_OPENMP
void App::rebuildGridParallel(int maxThreads) {
    const int totalCells = gw_ * gh_;
    if (totalCells <= 0) return;

    // Para problemas pequeños, la versión secuencial es más eficiente
    if (maxThreads <= 1 || cfg_.n < 2000 || totalCells < 16) {
        rebuildGridSequential();
        return;
    }

    if (particleCellIds_.size() != static_cast<size_t>(cfg_.n)) {
        particleCellIds_.resize(cfg_.n);
    }

    // Limitar número de hilos
    maxThreads = std::max(1, std::min(maxThreads, totalCells));
    const size_t required =
        static_cast<size_t>(totalCells) * static_cast<size_t>(maxThreads);

    if (perThreadCounts_.size() < required) {
        perThreadCounts_.assign(required, 0);
    } else {
        std::fill(perThreadCounts_.begin(),
                  perThreadCounts_.begin() + required, 0);
    }

    if (perThreadOffsets_.size() < required) {
        perThreadOffsets_.resize(required);
    }

    int activeThreads = 1;

    // 1) Conteos por hilo y celda
    #pragma omp parallel num_threads(maxThreads)
    {
        const int tid = omp_get_thread_num();
        int* localCounts = perThreadCounts_.data() +
                           static_cast<size_t>(tid) * totalCells;

        #pragma omp single
        {
            activeThreads = omp_get_num_threads();
        }

        #pragma omp for schedule(static)
        for (int i = 0; i < cfg_.n; ++i) {
            int cx = static_cast<int>(particles_[i].x / cellSize_);
            if (cx < 0) cx = 0;
            else if (cx >= gw_) cx = gw_ - 1;

            int cy = static_cast<int>(particles_[i].y / cellSize_);
            if (cy < 0) cy = 0;
            else if (cy >= gh_) cy = gh_ - 1;

            const int id = cellId(cx, cy);
            particleCellIds_[i] = id;
            localCounts[id] += 1;
        }
    }

    // 2) Reducir conteos a cellCounts_
    #pragma omp parallel for schedule(static) num_threads(activeThreads)
    for (int cell = 0; cell < totalCells; ++cell) {
        int sum = 0;
        for (int t = 0; t < activeThreads; ++t) {
            sum += perThreadCounts_[static_cast<size_t>(t) * totalCells + cell];
        }
        cellCounts_[cell] = sum;
    }

    // 3) Prefix sums globales
    cellOffsets_[0] = 0;
    for (int c = 0; c < totalCells; ++c) {
        cellOffsets_[c + 1] = cellOffsets_[c] + cellCounts_[c];
    }

    // 4) Offsets por hilo
    #pragma omp parallel for schedule(static) num_threads(activeThreads)
    for (int cell = 0; cell < totalCells; ++cell) {
        int base = cellOffsets_[cell];
        for (int t = 0; t < activeThreads; ++t) {
            const size_t idx = static_cast<size_t>(t) * totalCells + cell;
            perThreadOffsets_[idx] = base;
            base += perThreadCounts_[idx];
        }
    }

    // 5) Escribir índices en cellItems_
    #pragma omp parallel num_threads(activeThreads)
    {
        const int tid = omp_get_thread_num();
        int* writeOffsets = perThreadOffsets_.data() +
                            static_cast<size_t>(tid) * totalCells;

        #pragma omp for schedule(static)
        for (int i = 0; i < cfg_.n; ++i) {
            const int id  = particleCellIds_[i];
            const int pos = writeOffsets[id]++;
            cellItems_[pos] = i;
        }
    }
}
#endif


// versión sequencial
void App::buildEdgesSeq(float dt) {
    const int   winW   = cfg_.width;
    const int   winH   = cfg_.height;
    const float r2     = radius2_;
    const float invR2  = invRadius2_;

    const float s      = (rotationSign_ ? std::sin(rotationSign_ * rotationSpeed_ * dt) : 0.f);
    const float c      = (rotationSign_ ? std::cos(rotationSign_ * rotationSpeed_ * dt) : 1.f);

    // Actualizar partículas
    for (auto& p : particles_) {
        p.update(dt, winW, winH, cfg_.speed);
        if (rotationSign_) {
            p.rotateAroundSC(winW * 0.5f, winH * 0.5f, s, c);
        }
    }

    // Rehacer el grid
    rebuildGridSequential();

    // Limpieza inicial
    edges_.clear();
    edges_.reserve(static_cast<size_t>(cfg_.n) * 8);

    const int OFFSETX[5] = {0, 1, 1, 0, -1};
    const int OFFSETY[5] = {0, 0, 1, 1,  1};
    const int totalCells = gw_ * gh_;

    for (int cellIdFlat = 0; cellIdFlat < totalCells; ++cellIdFlat) {
        const int cellX = cellIdFlat % gw_;
        const int cellY = cellIdFlat / gw_;
        const int cellStart = cellOffsets_[cellIdFlat];
        const int cellEnd   = cellOffsets_[cellIdFlat + 1];

        for (int k = 0; k < 5; ++k) {
            const int neighborX = cellX + OFFSETX[k];
            const int neighborY = cellY + OFFSETY[k];

            if (neighborX < 0 || neighborX >= gw_ ||
                neighborY < 0 || neighborY >= gh_) {
                continue;
            }

            const int neighborId    = neighborY * gw_ + neighborX;
            const int neighborStart = cellOffsets_[neighborId];
            const int neighborEnd   = cellOffsets_[neighborId + 1];

            if (k == 0) {
                // Pares dentro de la misma celda: i<j
                for (int idxA = cellStart; idxA < cellEnd; ++idxA) {
                    for (int idxB = idxA + 1; idxB < cellEnd; ++idxB) {
                        const int pA = cellItems_[idxA];
                        const int pB = cellItems_[idxB];

                        const float dx = particles_[pA].x - particles_[pB].x;
                        const float dy = particles_[pA].y - particles_[pB].y;
                        const float d2 = dx*dx + dy*dy;

                        if (d2 <= r2) {
                            edges_.push_back({ pA, pB, 1.f - d2 * invR2 });
                        }
                    }
                }
            } else {
                // Pares entre celda actual y vecina
                for (int idxA = cellStart; idxA < cellEnd; ++idxA) {
                    const int pA = cellItems_[idxA];
                    for (int idxB = neighborStart; idxB < neighborEnd; ++idxB) {
                        const int pB = cellItems_[idxB];

                        const float dx = particles_[pA].x - particles_[pB].x;
                        const float dy = particles_[pA].y - particles_[pB].y;
                        const float d2 = dx*dx + dy*dy;

                        if (d2 <= r2) {
                            edges_.push_back({ pA, pB, 1.f - d2 * invR2 });
                        }
                    }
                }
            }
        }
    }
}

// versión paralela - simplified to match sequential structure
void App::buildEdgesPar(float dt) {
#ifndef USE_OPENMP
    // Si no hay OpenMP, usamos directamente la versión secuencial
    buildEdgesSeq(dt);
    return;
#else
    const int   winW   = cfg_.width;
    const int   winH   = cfg_.height;
    const float r2     = radius2_;
    const float invR2  = invRadius2_;
    const float s      = (rotationSign_ ? std::sin(rotationSign_ * rotationSpeed_ * dt) : 0.f);
    const float c      = (rotationSign_ ? std::cos(rotationSign_ * rotationSpeed_ * dt) : 1.f);

    const int totalCells = gw_ * gh_;

    // Elegir número de hilos a usar
    int maxThreads = std::max(1, omp_get_max_threads());
    if (cfg_.threads > 0 && cfg_.threads < maxThreads)
        maxThreads = cfg_.threads;

    if (maxThreads > totalCells)
        maxThreads = totalCells;

    // Problemas pequeños → usar secuencial
    if (maxThreads <= 1 || cfg_.n < 2000 || totalCells < 16) {
        buildEdgesSeq(dt);
        return;
    }

    // 1) Actualizar partículas en paralelo
    #pragma omp parallel for schedule(static) num_threads(maxThreads)
    for (int i = 0; i < cfg_.n; ++i) {
        particles_[i].update(dt, winW, winH, cfg_.speed);
        if (rotationSign_) {
            particles_[i].rotateAroundSC(winW * 0.5f, winH * 0.5f, s, c);
        }
    }

    // 2) Rehacer grid en paralelo
    rebuildGridParallel(maxThreads);

    // 3) Construir aristas con buffers por hilo
    static std::vector<std::vector<Edge>> threadEdges;
    if ((int)threadEdges.size() != maxThreads) {
        threadEdges.assign(maxThreads, std::vector<Edge>());
    }

    const size_t approxEdges    = static_cast<size_t>(cfg_.n) * 8;
    const size_t perThreadReserve =
        approxEdges / static_cast<size_t>(maxThreads) + 256;

    for (int t = 0; t < maxThreads; ++t) {
        if (threadEdges[t].capacity() < perThreadReserve) {
            threadEdges[t].reserve(perThreadReserve);
        }
    }

    const int OFFSETX[5] = {0, 1, 1, 0, -1};
    const int OFFSETY[5] = {0, 0, 1, 1,  1};
    const Particle* particlesPtr = particles_.data();

    int activeThreads = maxThreads;

    #pragma omp parallel num_threads(maxThreads)
    {
        const int tid = omp_get_thread_num();
        auto& localEdges = threadEdges[tid];
        localEdges.clear();

        const int totalCellsLocal = gw_ * gh_;

        #pragma omp single
        {
            activeThreads = omp_get_num_threads();
        }

        #pragma omp for schedule(guided, 8)
        for (int cellIdFlat = 0; cellIdFlat < totalCellsLocal; ++cellIdFlat) {
            const int cellX = cellIdFlat % gw_;
            const int cellY = cellIdFlat / gw_;
            const int cellStart = cellOffsets_[cellIdFlat];
            const int cellEnd   = cellOffsets_[cellIdFlat + 1];

            for (int k = 0; k < 5; ++k) {
                const int neighborX = cellX + OFFSETX[k];
                const int neighborY = cellY + OFFSETY[k];

                if (neighborX < 0 || neighborX >= gw_ ||
                    neighborY < 0 || neighborY >= gh_) {
                    continue;
                }

                const int neighborId    = neighborY * gw_ + neighborX;
                const int neighborStart = cellOffsets_[neighborId];
                const int neighborEnd   = cellOffsets_[neighborId + 1];

                if (k == 0) {
                    // Pares dentro de la misma celda
                    for (int idxA = cellStart; idxA < cellEnd; ++idxA) {
                        for (int idxB = idxA + 1; idxB < cellEnd; ++idxB) {
                            const int pA = cellItems_[idxA];
                            const int pB = cellItems_[idxB];

                            const float dx = particlesPtr[pA].x - particlesPtr[pB].x;
                            const float dy = particlesPtr[pA].y - particlesPtr[pB].y;
                            const float d2 = dx*dx + dy*dy;

                            if (d2 <= r2) {
                                localEdges.push_back({ pA, pB, 1.f - d2 * invR2 });
                            }
                        }
                    }
                } else {
                    // Pares entre celda actual y vecina
                    for (int idxA = cellStart; idxA < cellEnd; ++idxA) {
                        const int pA = cellItems_[idxA];
                        for (int idxB = neighborStart; idxB < neighborEnd; ++idxB) {
                            const int pB = cellItems_[idxB];

                            const float dx = particlesPtr[pA].x - particlesPtr[pB].x;
                            const float dy = particlesPtr[pA].y - particlesPtr[pB].y;
                            const float d2 = dx*dx + dy*dy;

                            if (d2 <= r2) {
                                localEdges.push_back({ pA, pB, 1.f - d2 * invR2 });
                            }
                        }
                    }
                }
            }
        }
    }

    // 4) Unir todas las aristas de los hilos
    size_t totalSize = 0;
    for (int t = 0; t < activeThreads; ++t) {
        totalSize += threadEdges[t].size();
    }

    edges_.resize(totalSize);
    size_t offset = 0;
    for (int t = 0; t < activeThreads; ++t) {
        auto& vec = threadEdges[t];
        if (!vec.empty()) {
            std::copy(vec.begin(), vec.end(), edges_.begin() + offset);
            offset += vec.size();
        }
    }
#endif
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

    if (cfg_.parallel) buildEdgesPar(dt);
    else               buildEdgesSeq(dt);

    setWindowTitle(timer_.fps());
}

void App::render() {
    if (cfg_.bench) return;

    if (g_whiteBg) SDL_SetRenderDrawColor(renderer_, 245, 245, 247, 255);
    else           SDL_SetRenderDrawColor(renderer_,  10,  10,  12, 255);
    SDL_RenderClear(renderer_);

    // Optimized batch rendering (sequential - SDL is not thread-safe)
    const size_t numEdges = edges_.size();
    if (numEdges > 0) {
        constexpr int NUM_BUCKETS = 8;
        struct EdgeLine {
            int x1, y1, x2, y2;
        };
        static std::vector<EdgeLine> bucketLines[NUM_BUCKETS];

        static bool bucketsInitialized = false;
        if (!bucketsInitialized) {
            for (int i = 0; i < NUM_BUCKETS; ++i) {
                bucketLines[i].reserve(4000);
            }
            bucketsInitialized = true;
        }

        // Clear buckets
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            bucketLines[i].clear();
        }

        const Particle* __restrict__ particlesPtr = particles_.data();
        const Edge* __restrict__ edgesPtr = edges_.data();

        // Bucket edges (sequential but optimized)
        for (size_t i = 0; i < numEdges; ++i) {
            const Edge& e = edgesPtr[i];
            const Particle& a = particlesPtr[e.a];
            const Particle& b = particlesPtr[e.b];

            int bucket = static_cast<int>(e.w * NUM_BUCKETS);
            if (bucket >= NUM_BUCKETS) bucket = NUM_BUCKETS - 1;

            bucketLines[bucket].push_back({
                static_cast<int>(a.x), static_cast<int>(a.y),
                static_cast<int>(b.x), static_cast<int>(b.y)
            });
        }

        // Render each bucket
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            const std::vector<EdgeLine>& lines = bucketLines[i];
            const size_t lineCount = lines.size();
            if (lineCount == 0) continue;

            const float w = (i + 0.5f) / NUM_BUCKETS;
            const SDL_Color c = paletteColor(palette_, w);
            const Uint8 alpha = static_cast<Uint8>(40 + 200 * w);

            SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, alpha);

            const EdgeLine* __restrict__ linesPtr = lines.data();
            for (size_t j = 0; j < lineCount; ++j) {
                const EdgeLine& line = linesPtr[j];
                SDL_RenderDrawLine(renderer_, line.x1, line.y1, line.x2, line.y2);
            }
        }
    }

    // Render particles
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
