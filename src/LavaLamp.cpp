#include "LavaLamp.h"
#include <algorithm>

LavaLamp::LavaLamp(int w, int h)
    : width(w), height(h), hotTemp(80.0f), coldTemp(20.0f),
      mediumDensity(1.2f), spawnTimer(0.0f), rng(std::random_device{}()), xDist(50, w - 50) {
}

float LavaLamp::getTemperatureAt(float y) const {
    // Linear temperature gradient: hot at bottom (height), cold at top (0)
    float t = y / height;
    return coldTemp + (hotTemp - coldTemp) * t;
}

void LavaLamp::spawnBlob() {
    if (blobs.size() >= MAX_BLOBS) return;

    // Spawn at bottom with high temperature
    float x = xDist(rng);
    float y = height - 30.0f;
    float initialTemp = hotTemp + (rand() % 10 - 5);  // Random variation
    float mass = 1.0f + (rand() % 50) / 100.0f;      // Random mass

    blobs.emplace_back(x, y, initialTemp, mass);
}

void LavaLamp::applyPhysics(Blob& blob, float dt) {
    // Get ambient temperature at blob's position
    float ambientTemp = getTemperatureAt(blob.position.y);

    // Update blob temperature (Newton's cooling)
    blob.update(dt, ambientTemp);

    // Calculate buoyancy force
    float buoyancy = blob.getBuoyancyForce(mediumDensity);

    // Apply forces: F = ma => a = F/m
    float acceleration = buoyancy / blob.mass;

    // Add drag force (velocity dependent)
    const float dragCoeff = 0.1f;
    float drag = -dragCoeff * blob.velocity.y;

    // Update velocity
    blob.velocity.y += (acceleration + drag) * dt;

    // Limit velocity
    const float maxVelocity = 150.0f;
    if (blob.velocity.y > maxVelocity) blob.velocity.y = maxVelocity;
    if (blob.velocity.y < -maxVelocity) blob.velocity.y = -maxVelocity;

    // Update position
    blob.position = blob.position + blob.velocity * dt;

    // Boundary checks - bounce off top/bottom
    if (blob.position.y < blob.radius) {
        blob.position.y = blob.radius;
        blob.velocity.y *= -0.3f;  // Some damping
    }
    if (blob.position.y > height - blob.radius) {
        blob.position.y = height - blob.radius;
        blob.velocity.y *= -0.3f;
    }
}

void LavaLamp::update(float dt) {
    // Spawn new blobs periodically
    spawnTimer += dt;
    if (spawnTimer >= SPAWN_INTERVAL) {
        spawnBlob();
        spawnTimer = 0.0f;
    }

    // Update all blobs
    for (auto& blob : blobs) {
        applyPhysics(blob, dt);
    }

    // Remove blobs that are too old or stable
    blobs.erase(
        std::remove_if(blobs.begin(), blobs.end(), [this](const Blob& b) {
            // Remove if outside bounds significantly
            return b.position.y < -100 || b.position.y > height + 100;
        }),
        blobs.end()
    );
}
