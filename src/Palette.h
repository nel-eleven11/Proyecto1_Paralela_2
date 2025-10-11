#ifndef PALETTE_H
#define PALETTE_H

#include <cstdint>
#include <string>

enum class PaletteKind { RED, BLUE, ORANGE, GREEN, PURPLE, RAINBOW };

struct RGB { uint8_t r, g, b; };

PaletteKind parsePalette(const std::string& s);
RGB         pickFromPalette(PaletteKind p, float u); // u in [0,1]

#endif
