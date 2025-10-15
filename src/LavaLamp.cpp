#include "LavaLamp.h"
#include <numeric>
#include <limits>

void LavaLamp::accumulateCenterAndRadius(Blob& b, const std::vector<Molecule>& mols) {
    if (b.members.empty()) { b.center = {0,0}; b.approxRadius = 0.0f; return; }

    double sx=0.0, sy=0.0;
    for (auto idx : b.members) { sx += mols[idx].position.x; sy += mols[idx].position.y; }
    b.center.x = float(sx / double(b.members.size()));
    b.center.y = float(sy / double(b.members.size()));

    double acc = 0.0;
    for (auto idx : b.members) {
        float dx = mols[idx].position.x - b.center.x;
        float dy = mols[idx].position.y - b.center.y;
        acc += std::sqrt(dx*dx + dy*dy);
    }
    b.approxRadius = float(acc / double(b.members.size()));
}

float LavaLamp::archYAtX(float x) const {
    float archBaseY = height * archBaseYFactor;
    float leftEdge  = archInsetX;
    float rightEdge = float(width) - archInsetX;
    if (x <= leftEdge)  return archBaseY + archSlope * (leftEdge - hotspotCenterX);
    if (x >= rightEdge) return archBaseY + archSlope * (rightEdge - hotspotCenterX);
    float d = std::abs(x - hotspotCenterX);
    return archBaseY + archSlope * d;
}

float LavaLamp::rampYAtX(float x) const {
    float ySide = height * rampStartYFactor;
    float yBasin= height * basinYFactor;
    float halfSpanPx = std::max(20.0f, float(width) * rampHalfSpanX);
    float t = std::abs(x - hotspotCenterX) / halfSpanPx;
    if (t > 1.0f) t = 1.0f;
    float weight = std::pow(t, rampCurvePower);
    return yBasin + (ySide - yBasin) * weight;
}

LavaLamp::LavaLamp(int w, int h, int numMolecules)
    : width(w), height(h),
      hotTemp(100.0f), coldTemp(20.0f),
      mediumDensity(1.0f), spawnTimer(0.0f),
      hotspotCenterX(float(w) * 0.5f),
      hotspotCenterY(float(h) * 0.85f),
      rng(std::mt19937{std::random_device{}()}),
      xDist(50.0f, float(w - 50)),
      uni01(0.0f, 1.0f)
{
    molecules.reserve(std::max(0, numMolecules));
    for (int i = 0; i < numMolecules; ++i) spawnMolecule();
    // initial grouping
    rebuildBlobsPaddingBased();
}

float LavaLamp::getTemperatureAt(float x, float y) const {
    float baseT = coldTemp + (hotTemp - coldTemp) * std::clamp(y / float(height), 0.0f, 1.0f);

    float yTop    = height * columnTopFactor;
    float yBottom = height * columnBottomFactor;
    float inBand  = (y >= yTop && y <= yBottom) ? 1.0f : 0.0f;

    float dx       = std::abs(x - hotspotCenterX);
    float coreZone = std::max(0.0f, 1.0f - dx / std::max(1.0f, columnHalfWidth));
    float feather  = std::clamp((columnHalfWidth + columnEdgeFeather - dx) / std::max(1.0f, columnEdgeFeather), 0.0f, 1.0f);

    float columnHeat = (columnHeatBonus * feather + columnCoreHeatBonus * coreZone) * inBand;
    return baseT + columnHeat;
}

void LavaLamp::spawnMolecule() {
    float yBottom = height * columnBottomFactor;
    float spawnHalfW = std::max(10.0f, columnHalfWidth * 0.6f);

    std::uniform_real_distribution<float> spawnX(hotspotCenterX - spawnHalfW, hotspotCenterX + spawnHalfW);
    std::uniform_real_distribution<float> spawnY(yBottom - 0.10f * height,    yBottom - 0.02f * height);

    float x = std::clamp(spawnX(rng), 0.0f, float(width));
    float y = std::min(float(height - 5.0f), spawnY(rng));

    float initialTemp = hotTemp + 10.0f;
    float mass = 0.35f + float(rand() % 30) / 200.0f;

    molecules.emplace_back(x, y, initialTemp, mass);
    molecules.back().blobId = -1;
}

void LavaLamp::handleWalls(Molecule& m) {
    if (m.position.x < m.radius) {
        m.position.x = m.radius;
        m.velocity.x = -m.velocity.x * wallRestitution;
        if (std::abs(m.velocity.x) < 0.1f) m.velocity.x = 0.0f;
    }
    if (m.position.x > width - m.radius) {
        m.position.x = width - m.radius;
        m.velocity.x = -m.velocity.x * wallRestitution;
        if (std::abs(m.velocity.x) < 0.1f) m.velocity.x = 0.0f;
    }
    if (m.position.y < m.radius) {
        m.position.y = m.radius;
        if (uni01(rng) < wallStickTop) m.velocity.y = 0.0f;
        else                           m.velocity.y = -m.velocity.y * wallRestitution;
    }
    if (m.position.y > height - m.radius) {
        m.position.y = height - m.radius;
        if (uni01(rng) < wallStickBottom) m.velocity.y = 0.0f;
        else                              m.velocity.y = -m.velocity.y * wallRestitution;
    }

    if (std::abs(m.velocity.x) < 0.1f && (m.position.x < m.radius || m.position.x > width - m.radius)) {
        m.velocity.x = 0.0f; 
    }

    if (std::abs(m.velocity.y) < 0.1f && (m.position.y < m.radius || m.position.y > height - m.radius)) {
        m.velocity.y = 0.0f;  
    }

    float yRamp = rampYAtX(m.position.x);
    if (m.position.y > yRamp + rampThickness) {
        m.position.y = yRamp + rampThickness;
        if (m.position.x < hotspotCenterX && m.velocity.x < 0.0f)
            m.velocity.x = -m.velocity.x * rampReflectK;
        if (m.position.x > hotspotCenterX && m.velocity.x > 0.0f)
            m.velocity.x = -m.velocity.x * rampReflectK;
        m.velocity.y += rampDownPush * 0.6f;
        float kickDir = (m.position.x < hotspotCenterX) ? 1.0f : -1.0f;
        m.velocity.x += kickDir * rampCenterKick;
    }

    float archY = archYAtX(m.position.x);
    if (m.position.y < archY + archThickness) {
        float dir = (m.position.x < hotspotCenterX) ? -1.0f : 1.0f;
        m.velocity.x += dir * archPushSide;
        m.velocity.y += archPushDown;
        if (m.velocity.y < 0.0f) m.velocity.y = 0.0f;
    }
}

void LavaLamp::applyPhysics(Molecule& m, float dt) {
    float ambientTemp = getTemperatureAt(m.position.x, m.position.y);
    m.update(dt, ambientTemp);

    m.padding = paddingFactor * m.radius;

    float tempDiff = m.temperature - ambientTemp;
    const float buoyancyStrength = 300.0f;
    float buoyancy = -tempDiff * buoyancyStrength;
    float acceleration = buoyancy / m.mass;

    const float dragCoeff = 0.15f;
    float drag = -dragCoeff * m.velocity.y * std::abs(m.velocity.y);

    float dxCenter = m.position.x - hotspotCenterX;
    bool insideCenterColumn = (std::abs(dxCenter) <= centerColumnHalfW);

    float updraftStartY = height * updraftStartFactor;
    if (insideCenterColumn && m.position.y > updraftStartY) {
        float range = std::max(1.0f, height - updraftStartY);
        float updraftScale = std::clamp((m.position.y - updraftStartY) / range, 0.0f, 1.0f);
        m.velocity.y -= updraftPush * updraftScale * dt;
        if (m.velocity.y > -centerLiftMinUp) m.velocity.y = -centerLiftMinUp;
        float jitter = (uni01(rng) * 2.0f - 1.0f) * hotJitterX;
        m.velocity.x += jitter * dt;
    } else {
        if (m.velocity.y < 0.0f) m.velocity.y *= outsideUpwardDampen;
    }

    float dyToHotspot = std::abs(m.position.y - hotspotCenterY);
    if (dyToHotspot < basinLiftRadiusY && insideCenterColumn) {
        float liftScale = 1.0f - (dyToHotspot / basinLiftRadiusY);
        m.velocity.y -= basinLiftBoost * liftScale * dt;
    }

    float intakeY = height * intakeYFactor;
    if (m.position.y > intakeY && !insideCenterColumn) {
        float dir = (dxCenter <= 0.0f) ? 1.0f : -1.0f;
        m.velocity.x += dir * intakePull * dt;
        if (m.velocity.y < 0.0f) m.velocity.y = 0.0f;
    }

    float bandTop = height * centerLineBandTopFactor;
    float bandBottom = height * centerLineBandBottomFactor;
    if (m.position.y > bandTop && m.position.y < bandBottom) {
        float toCenterDir = (dxCenter <= 0.0f) ? 1.0f : -1.0f;
        float distanceX = std::abs(dxCenter);
        float weightX   = std::clamp(1.0f - distanceX / std::max(1.0f, columnHalfWidth), 0.0f, 1.0f);
        float heatSeek  = centerLinePull * (0.5f + 0.5f * std::clamp((m.temperature - ambientTemp) / 50.0f, 0.0f, 1.0f));
        m.velocity.x += toCenterDir * heatSeek * (1.0f - weightX) * dt;
    }

    float topBandY = height * topOutflowBandFactor;
    if (m.position.y < topBandY && insideCenterColumn) {
        float outDir = (dxCenter <= 0.0f) ? -1.0f : 1.0f;
        m.velocity.x += outDir * topOutflowSidePush * dt;
    }

    float nfTop = height * centerNoFallBandTopFactor;
    float nfBottom = height * centerNoFallBandBottomFactor;
    if (insideCenterColumn && m.position.y > nfTop && m.position.y < nfBottom && m.velocity.y > 0.0f) {
        float outDir = (dxCenter <= 0.0f) ? -1.0f : 1.0f;
        m.velocity.x += outDir * centerNoFallSidePush * dt;
        if (m.velocity.y > 0.0f) m.velocity.y = 0.0f;
    }

    bool nearLeft  = (m.position.x < sideBandWidth);
    bool nearRight = (m.position.x > width - sideBandWidth);
    if (nearLeft || nearRight) {
        float boost = (m.position.y < height * 0.5f) ? sideTopBoost : 1.0f;
        m.velocity.y += sideDownPush * boost * dt;
        if (m.velocity.y > 0.0f) {
            float dirToCenter = (dxCenter <= 0.0f) ? 1.0f : -1.0f;
            m.velocity.x += dirToCenter * (rampCenterPull * 0.6f) * dt;
        }
        if (m.velocity.y < 0.0f) m.velocity.y = 0.0f;
    }

    m.velocity.x *= horizontalDamping;
    if (m.velocity.x >  maxHorizontalSpeed) m.velocity.x =  maxHorizontalSpeed;
    if (m.velocity.x < -maxHorizontalSpeed) m.velocity.x = -maxHorizontalSpeed;

    const float dampingFactor = 0.985f;
    m.velocity.y = (m.velocity.y + (acceleration + drag) * dt) * dampingFactor;

    const float maxVelocity = 180.0f;
    if (m.velocity.y >  maxVelocity) m.velocity.y =  maxVelocity;
    if (m.velocity.y < -maxVelocity) m.velocity.y = -maxVelocity;

    m.position = m.position + m.velocity * dt;

    handleWalls(m);
}

void LavaLamp::resolveMoleculeCollisions() {
    const float restitution = 0.2f;

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

                // positional correction (mass-weighted)
                float overlap = (minDist - dist);
                float totalMass = a.mass + b.mass;
                a.position.x -= nx * overlap * (b.mass / totalMass);
                a.position.y -= ny * overlap * (b.mass / totalMass);
                b.position.x += nx * overlap * (a.mass / totalMass);
                b.position.y += ny * overlap * (a.mass / totalMass);

                // inelastic impulse (no merge here; grouping is padding-based now)
                float dvx = b.velocity.x - a.velocity.x;
                float dvy = b.velocity.y - a.velocity.y;
                float dvn = dvx * nx + dvy * ny;
                if (dvn < 0.0f) {
                    float impulse = -(1.0f + restitution) * dvn / (1.0f / a.mass + 1.0f / b.mass);
                    a.velocity.x -= impulse * nx / a.mass;
                    a.velocity.y -= impulse * ny / a.mass;
                    b.velocity.x += impulse * nx / b.mass;
                    b.velocity.y += impulse * ny / b.mass;
                }
            }
        }
    }
}

void LavaLamp::rebuildBlobsPaddingBased() {
    const int N = (int)molecules.size();
    if (N == 0) { blobs.clear(); return; }

    // Union-Find
    std::vector<int> parent(N);
    std::iota(parent.begin(), parent.end(), 0);
    auto findp = [&](int a) {
        int r = a;
        while (parent[r] != r) r = parent[r];
        while (parent[a] != a) { int p = parent[a]; parent[a] = r; a = p; }
        return r;
    };
    auto unite = [&](int a, int b) {
        a = findp(a); b = findp(b); if (a == b) return;
        parent[b] = a;
    };

    for (int i = 0; i < N; ++i) {
        const Molecule& A = molecules[i];
        for (int j = i+1; j < N; ++j) {
            const Molecule& B = molecules[j];
            float dx = B.position.x - A.position.x;
            float dy = B.position.y - A.position.y;
            float dist2 = dx*dx + dy*dy;

            float thr = (A.radius + B.radius) + linkPaddingFraction * (A.padding + B.padding);
            if (dist2 <= thr*thr) {
                unite(i, j);
            }
        }
    }

    // build blobs map
    blobs.clear();
    std::unordered_map<int,int> rootToBlobId;
    int nextId = 0;
    for (int i = 0; i < N; ++i) {
        int r = findp(i);
        auto it = rootToBlobId.find(r);
        int bid;
        if (it == rootToBlobId.end()) {
            bid = nextId++;
            rootToBlobId[r] = bid;
            blobs[bid] = Blob{ bid };
        } else {
            bid = it->second;
        }
        blobs[bid].members.push_back(i);
        molecules[i].blobId = bid;
    }

    // compute center & radius per blob
    for (auto& kv : blobs) {
        accumulateCenterAndRadius(kv.second, molecules);
    }
}

void LavaLamp::updateClusters() {
    rebuildBlobsPaddingBased();
}

void LavaLamp::update(float dt) {
    spawnTimer += dt;
    if (spawnTimer >= SPAWN_INTERVAL) {
        spawnMolecule();
        spawnTimer = 0.0f;
    }

    for (auto& m : molecules) applyPhysics(m, dt);
    resolveMoleculeCollisions();
    updateClusters();
    molecules.erase(
        std::remove_if(molecules.begin(), molecules.end(), [this](const Molecule& m){
            return m.position.y < -200 || m.position.y > height + 200 ||
                   m.position.x < -200 || m.position.x > width  + 200;
        }),
        molecules.end()
    );
}
