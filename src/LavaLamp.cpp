#include "LavaLamp.h"
#include <cmath>

// --- local utility: recompute blob centroid & approx radius ---
static inline void accumulateCenterAndRadius(Blob& b, const std::vector<Molecule>& mols) {
    if (b.members.empty()) { b.center = {0,0}; b.approxRadius = 0.0f; return; }

    double sx = 0.0, sy = 0.0;
    for (auto idx : b.members) {
        sx += mols[idx].position.x;
        sy += mols[idx].position.y;
    }
    b.center.x = float(sx / double(b.members.size()));
    b.center.y = float(sy / double(b.members.size()));

    double acc = 0.0;
    for (auto idx : b.members) {
        float dx = mols[idx].position.x - b.center.x;
        float dy = mols[idx].position.y - b.center.y;
        acc += std::sqrt(dx*dx + dy*dy);
    }
    double meanDist = acc / double(b.members.size());
    b.approxRadius = float(meanDist);
}

LavaLamp::LavaLamp(int w, int h, int numMolecules)
    : width(w), height(h),
      hotTemp(100.0f), coldTemp(20.0f),
      mediumDensity(1.0f), spawnTimer(0.0f),
      rng(std::mt19937{std::random_device{}()}),
      xDist(50.0f, float(w - 50)),
      uni01(0.0f, 1.0f)
{
    molecules.reserve(std::max(0, numMolecules));
    for (int i = 0; i < numMolecules; ++i) spawnMolecule();
}

float LavaLamp::getTemperatureAt(float y) const {
    // Linear gradient: hot at bottom (height), cold at top (0)
    float t = std::clamp(y / float(height), 0.0f, 1.0f);
    return coldTemp + (hotTemp - coldTemp) * t;
}

void LavaLamp::spawnMolecule() {
    float x = xDist(rng);
    float y = height - 30.0f;
    float initialTemp = hotTemp + float((rand() % 20) - 10);
    float mass = 0.35f + float(rand() % 30) / 200.0f;

    molecules.emplace_back(x, y, initialTemp, mass);

    Molecule& m = molecules.back();
    m.blobId = nextBlobId++;
    Blob B; B.id = m.blobId; B.members.push_back(molecules.size() - 1);
    blobs[B.id] = std::move(B);
}

void LavaLamp::handleWalls(Molecule& m) {
    // Left/right walls (capsule)
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
        if (uni01(rng) < wallStickTop) m.velocity.y = 0.0f;
        else                           m.velocity.y = -m.velocity.y * wallRestitution;
    }

    // Bottom wall
    if (m.position.y > height - m.radius) {
        m.position.y = height - m.radius;
        if (uni01(rng) < wallStickBottom) m.velocity.y = 0.0f;
        else                              m.velocity.y = -m.velocity.y * wallRestitution;
    }
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
    if (m.velocity.y >  maxVelocity) m.velocity.y =  maxVelocity;
    if (m.velocity.y < -maxVelocity) m.velocity.y = -maxVelocity;

    m.position = m.position + m.velocity * dt;

    handleWalls(m);
}

void LavaLamp::resolveMoleculeCollisions() {
    const float restitution = 0.2f;     // inelastic to promote merging
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

                // Separate slightly to prevent sinking
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
                    int A = a.blobId, B = b.blobId;
                    if (A != B) {
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
    // Recompute centers/radii and shed rim molecules
    for (auto it = blobs.begin(); it != blobs.end(); ) {
        Blob& B = it->second;

        // Filter out dead members
        std::vector<size_t> filtered;
        filtered.reserve(B.members.size());
        for (auto idx : B.members) {
            if (idx < molecules.size() && molecules[idx].blobId == B.id)
                filtered.push_back(idx);
        }
        B.members.swap(filtered);

        if (B.members.empty()) {
            it = blobs.erase(it);
            continue;
        }

        accumulateCenterAndRadius(B, molecules);

        // Shedding: randomly detach rim molecules
        if (splitChance > 0.0f && B.approxRadius > 5.0f) {
            for (auto idx : B.members) {
                const Molecule& m = molecules[idx];
                float dx = m.position.x - B.center.x;
                float dy = m.position.y - B.center.y;
                float d  = std::sqrt(dx*dx + dy*dy);
                bool isRim = (d > 0.75f * B.approxRadius);

                if (isRim && uni01(rng) < splitChance) {
                    int newId = nextBlobId++;
                    blobs[newId] = Blob{ newId };
                    blobs[newId].members.push_back(idx);
                    // Note: update molecule blobId
                    const_cast<Molecule&>(molecules[idx]).blobId = newId;
                }
            }
        }
        ++it;
    }
}

void LavaLamp::update(float dt) {
    spawnTimer += dt;
    if (spawnTimer >= SPAWN_INTERVAL) {
        spawnMolecule();
        spawnTimer = 0.0f;
    }

    for (auto& m : molecules)  applyPhysics(m, dt);
    resolveMoleculeCollisions();
    updateClusters();

    // Cleanup (rare)
    molecules.erase(
        std::remove_if(molecules.begin(), molecules.end(), [this](const Molecule& m){
            return m.position.y < -200 || m.position.y > height + 200 ||
                   m.position.x < -200 || m.position.x > width  + 200;
        }),
        molecules.end()
    );
}
