// Utilities to recompute blob centroid and approximate radius.

#include "Blob.h"
#include <cmath>

static inline void accumulateCenterAndRadius(Blob& b, const std::vector<Molecule>& mols) {
    if (b.members.empty()) { b.center = {0,0}; b.approxRadius = 0.0f; return; }
    // Centroid
    double sx = 0.0, sy = 0.0;
    for (auto idx : b.members) { sx += mols[idx].position.x; sy += mols[idx].position.y; }
    b.center.x = float(sx / double(b.members.size()));
    b.center.y = float(sy / double(b.members.size()));
    // Approx radius as RMS distance to center + average molecule radius
    double acc = 0.0;
    for (auto idx : b.members) {
        float dx = mols[idx].position.x - b.center.x;
        float dy = mols[idx].position.y - b.center.y;
        acc += std::sqrt(dx*dx + dy*dy);
    }
    double meanDist = (b.members.empty()? 0.0 : acc / double(b.members.size()));
    b.approxRadius = float(meanDist);
}
