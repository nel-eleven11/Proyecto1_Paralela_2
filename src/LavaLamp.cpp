#include "LavaLamp.h"
#include <algorithm>
#include <cmath>

LavaLamp::LavaLamp(int w, int h, int numMolecules)
    : width(w), height(h), hotTemp(100.0f), coldTemp(20.0f),
      mediumDensity(1.0f), spawnTimer(0.0f),
      rng(std::random_device{}()), xDist(50, w - 50) {
    molecules.reserve(std::max(0, numMolecules));
    for (int i = 0; i < numMolecules; ++i) {
        spawnMolecule();
    }
}

float LavaLamp::getTemperatureAt(float y) const {
    // Linear temperature gradient: hot at bottom (height), cold at top (0)
    float t = y / height;
    return coldTemp + (hotTemp - coldTemp) * t;
}

void LavaLamp::spawnMolecule() {
    float x = xDist(rng);
    float y = height - 30.0f;
    float initialTemp = hotTemp + (rand() % 20 - 10);
    float mass = 0.35f + (rand() % 30) / 200.0f; 

    molecules.emplace_back(x, y, initialTemp, mass);

    // Create a new singleton blob for this molecule 
    Molecule& m = molecules.back();
    m.blobId = nextBlobId++;
    Blob B; B.id = m.blobId; B.members.push_back(molecules.size()-1);
    blobs[B.id] = std::move(B);
}

void LavaLamp::applyPhysics(Molecule& m, float dt) {
    float ambientTemp = getTemperatureAt(m.position.y);
    m.update(dt, ambientTemp);

    float tempDiff = m.temperature - ambientTemp;
    const float buoyancyStrength = 300.0f;
    float buoyancy = -tempDiff * buoyancyStrength;

    float acceleration = buoyancy / m.mass;

    const float dragCoeff = 0.15f;
    float drag = -dragCoeff * m.velocity.y * std::abs(m.velocity.y);

    const float dampingFactor = 0.985f;
    m.velocity.y = (m.velocity.y + (acceleration + drag) * dt) * dampingFactor;

    const float maxVelocity = 160.0f;
    if (m.velocity.y > maxVelocity) m.velocity.y = maxVelocity;
    if (m.velocity.y < -maxVelocity) m.velocity.y = -maxVelocity;

    m.position = m.position + m.velocity * dt;

    handleWalls(m); 
}

void LavaLamp::resolveMoleculeCollisions() {
    const float restitution = 0.2f; // inelastic to promote merging
    const float mergeSpeedThresh = 45.0f;

    for (size_t i = 0; i < molecules.size(); ++i) {
        for (size_t j = i + 1; j < molecules.size(); ++j) {
            Molecule& a = molecules[i];
            Molecule& b = molecules[j];

            float dx = b.position.x - a.position.x;
            float dy = b.position.y - a.position.y;
            float dist2 = dx*dx + dy*dy;
            float minDist = a.radius + b.radius;

            if (dist2 <= (minDist*minDist) && dist2 > 1e-4f) {
                float dist = std::sqrt(dist2);
                float nx = dx / dist;
                float ny = dy / dist;

                float dvx = b.velocity.x - a.velocity.x;
                float dvy = b.velocity.y - a.velocity.y;
                float dvn = dvx * nx + dvy * ny;

                // Separate slightly to avoid sinking
                float overlap = (minDist - dist);
                float totalMass = a.mass + b.mass;
                a.position.x -= nx * overlap * (b.mass / totalMass);
                a.position.y -= ny * overlap * (b.mass / totalMass);
                b.position.x += nx * overlap * (a.mass / totalMass);
                b.position.y += ny * overlap * (a.mass / totalMass);

                if (dvn > 0.0f) continue; // already separating

                // Inelastic impulse
                float impulse = -(1.0f + restitution) * dvn / (1.0f / a.mass + 1.0f / b.mass);
                a.velocity.x -= impulse * nx / a.mass;
                a.velocity.y -= impulse * ny / a.mass;
                b.velocity.x += impulse * nx / b.mass;
                b.velocity.y += impulse * ny / b.mass;

                // Merge chance based on stickiness and relative speed
                float relSpeed = std::fabs(dvn);
                if (relSpeed < mergeSpeedThresh && uni01(rng) < stickiness) {
                    // Merge cluster ids: assign b -> a's blob (or vice versa)
                    int A = a.blobId, B = b.blobId;
                    if (A != B) {
                        // Move all members of blob B into blob A
                        Blob& BA = blobs[A];
                        Blob& BB = blobs[B];
                        for (auto idx : BB.members) {
                            molecules[idx].blobId = A;
                            BA.members.push_back(idx);
                        }
                        BB.members.clear();
                        blobs.erase(B);
                    }
                }
            }
        }
    }
}

void LavaLamp::updateClusters() {
    // Recompute centers/radii
    for (auto& kv : blobs) {
        Blob& B = kv.second;
        // remove dead members
        std::vector<size_t> tmp;
        tmp.reserve(B.members.size());
        for (auto idx : B.members) {
            if (idx < molecules.size() && molecules[idx].blobId == B.id) tmp.push_back(idx);
        }
        B.members.swap(tmp);

        // recompute centroid and approx radius
        accumulateCenterAndRadius(B, molecules);

        // Shedding: randomly detach rim molecules
        if (!B.members.empty() && splitChance > 0.0f) {
            for (auto idx : B.members) {
                const Molecule& m = molecules[idx];
                float dx = m.position.x - B.center.x;
                float dy = m.position.y - B.center.y;
                float d = std::sqrt(dx*dx + dy*dy);
                bool isRim = (B.approxRadius > 5.0f) && (d > 0.75f * B.approxRadius);
                if (isRim && uni01(rng) < splitChance) {
                    // detach: assign new blob id
                    int newId = nextBlobId++;
                    blobs[newId] = Blob{newId};
                    blobs[newId].members.push_back(idx);
                    molecules[idx].blobId = newId;
                }
            }
        }
    }
}

void LavaLamp::handleWalls(Molecule& m) {
    // Left/right walls (hard capsule)
    if (m.position.x < m.radius) {
        m.position.x = m.radius;
        m.velocity.x = -m.velocity.x * wallRestitution;
    }
    if (m.position.x > width - m.radius) {
        m.position.x = width - m.radius;
        m.velocity.x = -m.velocity.x * wallRestitution;
    }

    // Top wall
    if (m.position.y < m.radius) {
        m.position.y = m.radius;
        // stick or bounce
        if (uni01(rng) < wallStickTop) {
            m.velocity.y = 0.0f; // temporary stick
        } else {
            m.velocity.y = -m.velocity.y * wallRestitution;
        }
    }

    // Bottom wall
    if (m.position.y > height - m.radius) {
        m.position.y = height - m.radius;
        if (uni01(rng) < wallStickBottom) {
            m.velocity.y = 0.0f;
        } else {
            m.velocity.y = -m.velocity.y * wallRestitution;
        }
    }
}



void LavaLamp::update(float dt) {
    spawnTimer += dt;
    if (spawnTimer >= SPAWN_INTERVAL) { spawnMolecule(); spawnTimer = 0.0f; }

    for (auto& m : molecules) applyPhysics(m, dt);
    resolveMoleculeCollisions();
    updateClusters();

    // Optional cleanup of molecules far outside lamp (rare with walls)
    molecules.erase(
        std::remove_if(molecules.begin(), molecules.end(), [this](const Molecule& m){
            return m.position.y < -200 || m.position.y > height + 200 ||
                   m.position.x < -200 || m.position.x > width + 200;
        }),
        molecules.end()
    );
}