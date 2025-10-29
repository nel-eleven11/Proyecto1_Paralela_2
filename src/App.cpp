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



void App::run() {
    bool running = true;
    while (running) {
        handleEvents(running);
        float dt = timer_.tick();
        if (!paused_) update(dt);
        render();
    }
}
