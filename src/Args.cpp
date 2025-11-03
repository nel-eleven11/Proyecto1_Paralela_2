#include "Args.h"
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>

static bool readInt(const char* s, int& out) {
    char* end=nullptr; long v = std::strtol(s, &end, 10);
    if (!end || *end!='\0') return false;
    out = (int)v; return true;
}
static bool readFloat(const char* s, float& out) {
    char* end=nullptr; float v = std::strtof(s, &end);
    if (!end || *end!='\0') return false;
    out = v; return true;
}

bool parseArgs(int argc, char** argv, Config& out, std::string& error) {
    for (int i=1;i<argc;i++) {
        std::string a = argv[i];
        auto need = [&](int i){ return i+1<argc; };

        if (a=="-h" || a=="--help") { printHelp(); return false; }
        else if (a=="-w"   && need(i)) { if(!readInt(argv[++i], out.width))  { error="Anchura inválida"; return false; } }
        else if (a=="-hgt" && need(i)) { if(!readInt(argv[++i], out.height)) { error="Altura inválida"; return false; } }
        else if (a=="-n"   && need(i)) { if(!readInt(argv[++i], out.n))      { error="n inválido"; return false; } }
        else if (a=="-r"   && need(i)) { if(!readFloat(argv[++i], out.radius)) { error="radio inválido"; return false; } }
        else if (a=="-s"   && need(i)) { if(!readFloat(argv[++i], out.speed))  { error="speed inválido"; return false; } }
        else if (a=="--seq") { out.parallel=false; }
        else if (a=="--par") { out.parallel=true; }
        else if (a=="--seed" && need(i)) {
            int tmp=0; if(!readInt(argv[++i], tmp)) { error="seed inválida"; return false; }
            out.seed = (unsigned int)tmp;
        }
        else if (a=="--threads" && need(i)) {
            if(!readInt(argv[++i], out.threads)) { error="threads inválido"; return false; }
        }
        else if (a=="--bench" && need(i)) {
            int b=0; if(!readInt(argv[++i], b)) { error="bench inválido"; return false; }
            out.bench = (b!=0);
        }
        else if (a=="--novsync") {
            out.novsync = true;
        }
        else {
            std::ostringstream oss; oss << "Flag no reconocida: " << a;
            error = oss.str(); return false;
        }
    }

    if (out.width < 640)  out.width  = 640;
    if (out.height < 480) out.height = 480;
    if (out.n <= 0)       out.n = 100;
    if (out.radius < 10)  out.radius = 10.f;
    if (out.speed <= 0)   out.speed  = 1.f;
    return true;
}

void printHelp() {
    std::cout <<
R"(Screensaver OpenMP (UVG)
Uso:
  ./omp_screensaver -n 1200 -w 1280 -hgt 720 -r 90 -s 1.0 --par --threads 8 --bench 0
Flags:
  -n, -w, -hgt, -r, -s        parámetros visuales
  --par / --seq               modo paralelo o secuencial
  --seed <int>                semilla RNG (opcional)
  --threads <K>               fuerza K hilos en OpenMP (opcional)
  --bench <0/1>               1 = NO dibuja (mide solo cómputo)
  --novsync                   Desactiva VSync (permite FPS > 60)

Controles:
  ↑/↓ radio, ←/→ velocidad, F1..F4 paletas,
  C auto-ciclo (ON/OFF), B cambia fondo,
  R rota (OFF → CW → CCW), Espacio pausa, ESC salir
)" << std::endl;
}
