#include "Palette.h"
#include <algorithm>
#include <cmath>

static const std::vector<RGB> P_RED    = {{220,40,50},{255,60,60},{200,20,30}};
static const std::vector<RGB> P_BLUE   = {{40,60,200},{60,120,255},{20,30,140}};
static const std::vector<RGB> P_ORANGE = {{255,120,40},{255,160,60},{220,90,20}};
static const std::vector<RGB> P_GREEN  = {{30,180,90},{60,220,140},{20,120,60}};
static const std::vector<RGB> P_PURPLE = {{160,60,220},{200,120,255},{100,40,180}};
static const std::vector<RGB> P_RAIN   = {{255,50,50},{255,150,50},{255,255,50},{50,255,50},{50,150,255},{160,50,255}};

PaletteKind parsePalette(const std::string& s) {
    std::string t = s; for (auto& c: t) c = std::tolower(c);
    if (t=="red") return PaletteKind::RED;
    if (t=="blue") return PaletteKind::BLUE;
    if (t=="orange") return PaletteKind::ORANGE;
    if (t=="green") return PaletteKind::GREEN;
    if (t=="purple") return PaletteKind::PURPLE;
    if (t=="rainbow" || t=="rain") return PaletteKind::RAINBOW;
    return PaletteKind::RED;
}

static inline RGB lerp(const RGB& a, const RGB& b, float t){
    auto L = [&](uint8_t A, uint8_t B){ return (uint8_t)std::round((1.0f-t)*A + t*B); };
    return { L(a.r,b.r), L(a.g,b.g), L(a.b,b.b) };
}

RGB pickFromPalette(PaletteKind p, float u) {
    const std::vector<RGB>* P = &P_RED;
    switch(p){
        case PaletteKind::BLUE:   P=&P_BLUE; break;
        case PaletteKind::ORANGE: P=&P_ORANGE; break;
        case PaletteKind::GREEN:  P=&P_GREEN; break;
        case PaletteKind::PURPLE: P=&P_PURPLE; break;
        case PaletteKind::RAINBOW:P=&P_RAIN; break;
        default: break;
    }
    if (P->size()==1) return (*P)[0];
    float f = std::clamp(u, 0.0f, 1.0f) * float(P->size()-1);
    int i = int(std::floor(f));
    int j = std::min(int(P->size()-1), i+1);
    float tt = f - float(i);
    return lerp((*P)[i], (*P)[j], tt);
}
