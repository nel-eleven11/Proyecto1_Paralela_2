#include "Molecule.h"
#include "Blob.h"
#include <unordered_map>

class LavaLamp {
public:
    LavaLamp(int width, int height, int numMolecules);

    void update(float dt);

    // expose molecules and blobs for rendering decisions
    const std::vector<Molecule>& getMolecules() const { return molecules; }
    const std::unordered_map<int, Blob>& getBlobs() const { return blobs; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getTemperatureAt(float y) const;

    // Parameters to control sticking/splitting
    void setStickiness(float s) { stickiness = std::max(0.0f, std::min(1.0f, s)); }
    void setSplitChance(float c) { splitChance = std::max(0.0f, std::min(1.0f, c)); }

private:
    int width, height;

    float wallRestitution = 0.25f;     // soft bounce on walls
    float wallStickTop    = 0.2f;      // chance to stick at the top
    float wallStickBottom = 0.05f;     // small chance to temporarily stick at bottom
    void  handleWalls(Molecule& m);

    std::vector<Molecule> molecules;           // base particles
    std::unordered_map<int, Blob> blobs;       // dynamic clusters
    int nextBlobId = 0;

    float hotTemp;
    float coldTemp;
    float mediumDensity;

    float spawnTimer;
    static constexpr float SPAWN_INTERVAL = 2.0f;

    // cluster dynamics
    float stickiness = 0.6f;   // merge likelihood on soft collisions
    float splitChance = 0.02f; // probability to shed a rim molecule per frame

    std::mt19937 rng;
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> uni01{0.0f, 1.0f};

    void spawnMolecule();
    void applyPhysics(Molecule& m, float dt);
    void resolveMoleculeCollisions(); // inelastic + stick/merge
    void updateClusters();            // recompute centers/radii and shed rim molecules
    void handleWalls(Molecule& m);    // see Part 3
};
