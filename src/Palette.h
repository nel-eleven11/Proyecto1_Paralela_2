#ifndef PALETTE_H
#define PALETTE_H

#include <string>
#include <vector>
#include <cstdint>

enum class PaletteKind { RED, BLUE, ORANGE, GREEN, PURPLE, RAINBOW };

struct RGB { uint8_t r, g, b; };

PaletteKind parsePalette(const std::string& s);
RGB pickFromPalette(PaletteKind p, float u); // u in [0,1] to pick pseudo-random color

#endif
