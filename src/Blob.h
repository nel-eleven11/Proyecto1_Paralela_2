// A Blob is a dynamic cluster of molecules. It has a changing shape and membership.

#ifndef BLOB_CLUSTER_H
#define BLOB_CLUSTER_H

#include <vector>
#include <cstddef>
#include "Molecule.h"

struct Blob {
    int id = -1;
    float stickiness = 0.6f;   // 0..1 probability factor to merge on impact
    float viscosity  = 0.25f;  // motion damping used when inside the cluster

    // Cached centroid and radius (updated each frame)
    Vec2  center{0,0};
    float approxRadius = 0.0f;

    // Indices of molecules belonging to this blob
    std::vector<size_t> members;
};

#endif
