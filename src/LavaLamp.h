#ifndef LAVALAMP_H
#define LAVALAMP_H

#include <vector>
#include <unordered_map>
#include <random>
#include <algorithm>
#include "Molecule.h"
#include "Blob.h"

class LavaLamp {
public:
    LavaLamp(int width, int height, int numMolecules);
    void update(float dt);

    const std::vector<Molecule>&          getMolecules() const { return molecules; }
    const std::unordered_map<int, Blob>&  getBlobs()     const { return blobs; }

    int   getWidth()  const { return width; }
    int   getHeight() const { return height; }

    float getTemperatureAt(float x, float y) const;

    void setStickiness(float s)  { stickiness  = std::max(0.0f, std::min(1.0f, s)); }
    void setSplitChance(float c) { splitChance = std::max(0.0f, std::min(1.0f, c)); }

private:
    int width, height;

    std::vector<Molecule>           molecules;
    std::unordered_map<int, Blob>   blobs;
    int nextBlobId = 0;

    float hotTemp;
    float coldTemp;
    float mediumDensity;

    float spawnTimer;
    static constexpr float SPAWN_INTERVAL = 2.0f;

    float stickiness  = 0.75f;
    float splitChance = 0.01f;

    float wallRestitution = 0.25f;
    float wallStickTop    = 0.35f;
    float wallStickBottom = 0.03f;

    const float hotspotCenterX;
    const float hotspotCenterY;

    float archBaseYFactor   = 0.16f;
    float archInsetX        = 90.0f;
    float archSlope         = 0.65f;
    float archPushSide      = 110.0f;
    float archPushDown      = 120.0f;
    float archThickness     = 12.0f;

    float rampStartYFactor  = 0.55f;
    float basinYFactor      = 0.82f;
    float rampHalfSpanX     = 0.48f;
    float rampCurvePower    = 2.0f;
    float rampThickness     = 10.0f;
    float rampReflectK      = 0.38f;
    float rampDownPush      = 80.0f;
    float rampCenterPull    = 170.0f;
    float rampCenterKick    = 60.0f;

    float centerColumnHalfW = 80.0f;
    float updraftStartFactor= 0.62f;
    float updraftPush       = 320.0f;
    float hotJitterX        = 10.0f;

    float basinLiftRadiusY  = 220.0f;
    float basinLiftBoost    = 260.0f;
    float centerLiftMinUp   = 140.0f;

    float outsideUpwardDampen = 0.06f;

    float sideBandWidth     = 90.0f;
    float sideDownPush      = 80.0f;
    float sideTopBoost      = 1.6f;

    float horizontalDamping = 0.90f;
    float maxHorizontalSpeed= 70.0f;

    float intakeYFactor     = 0.78f;
    float intakePull        = 220.0f;

    float centerLineBandTopFactor    = 0.35f;
    float centerLineBandBottomFactor = 0.88f;
    float centerLinePull             = 140.0f;

    float topOutflowBandFactor = 0.20f;
    float topOutflowSidePush   = 360.0f;

    float centerNoFallBandTopFactor    = 0.45f;
    float centerNoFallBandBottomFactor = 0.83f;
    float centerNoFallSidePush         = 460.0f;

    float columnTopFactor        = 0.20f;
    float columnBottomFactor     = 0.90f;
    float columnHalfWidth        = 70.0f;
    float columnEdgeFeather      = 40.0f;
    float columnHeatBonus        = 58.0f;
    float columnCoreHeatBonus    = 26.0f;

    std::mt19937 rng;
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> uni01;

    void spawnMolecule();
    void applyPhysics(Molecule& m, float dt);
    void resolveMoleculeCollisions();
    void updateClusters();
    void handleWalls(Molecule& m);

    float archYAtX(float x) const;
    float rampYAtX(float x) const;
};

#endif
