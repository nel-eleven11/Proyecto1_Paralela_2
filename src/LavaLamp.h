#ifndef LAVALAMP_H
#define LAVALAMP_H

#include "Molecule.h"
#include <vector>
#include <random>

class LavaLamp {
public:
    LavaLamp(int width, int height, int numMolecules);

    void update(float dt);
    const std::vector<Molecule>& getMolecules() const { return molecules; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Get temperature at a given y position
    float getTemperatureAt(float y) const;

private:
    int width, height;
    std::vector<Molecule> molecules;

    float hotTemp;          // Temperature at bottom
    float coldTemp;         // Temperature at top
    float mediumDensity;    // Density of the surrounding medium

    float spawnTimer;
    static constexpr float SPAWN_INTERVAL = 2.0f;  // Spawn molecule every 2 seconds
    static constexpr int MAX_MOLECULES = 20000000;

    std::mt19937 rng;
    std::uniform_real_distribution<float> xDist;

    void spawnMolecule();
    void applyPhysics(Molecule& molecule, float dt);
    void handleCollisions();
};

#endif
