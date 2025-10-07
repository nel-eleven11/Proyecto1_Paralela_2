#ifndef LAVALAMP_H
#define LAVALAMP_H

#include "Blob.h"
#include <vector>
#include <random>

class LavaLamp {
public:
    LavaLamp(int width, int height);

    void update(float dt);
    const std::vector<Blob>& getBlobs() const { return blobs; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Get temperature at a given y position
    float getTemperatureAt(float y) const;

private:
    int width, height;
    std::vector<Blob> blobs;

    float hotTemp;          // Temperature at bottom
    float coldTemp;         // Temperature at top
    float mediumDensity;    // Density of the surrounding medium

    float spawnTimer;
    static constexpr float SPAWN_INTERVAL = 2.0f;  // Spawn blob every 2 seconds
    static constexpr int MAX_BLOBS = 20;

    std::mt19937 rng;
    std::uniform_real_distribution<float> xDist;

    void spawnBlob();
    void applyPhysics(Blob& blob, float dt);
};

#endif
