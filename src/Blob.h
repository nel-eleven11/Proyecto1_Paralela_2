#ifndef BLOB_CLUSTER_H
#define BLOB_CLUSTER_H

#include <vector>
#include "Molecule.h"

struct Blob {
    int   id = -1;
    float stickiness = 0.6f;  // merge likelihood on impact (0..1)
    float viscosity  = 0.25f; // motion damping while in cluster

    // Cached geometric estimates (updated each frame)
    Vec2  center{0,0};
    float approxRadius = 0.0f;

    // Members (indices into the global molecules vector)
    std::vector<size_t> members;
};

#endif
