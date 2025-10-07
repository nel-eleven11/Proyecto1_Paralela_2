#include "LavaLamp.h"
#include <algorithm>
#include <cmath>

LavaLamp::LavaLamp(int w, int h)
    : width(w), height(h), hotTemp(100.0f), coldTemp(20.0f),
      mediumDensity(1.0f), spawnTimer(0.0f), rng(std::random_device{}()), xDist(50, w - 50) {
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
    float initialTemp = hotTemp + (rand() % 20 - 10);  // Random variation
    float mass = 0.5f + (rand() % 30) / 100.0f;        // Light mass: 0.5-0.8

    blobs.emplace_back(x, y, initialTemp, mass);
}

void LavaLamp::applyPhysics(Blob& blob, float dt) {
    // Get ambient temperature at blob's position
    float ambientTemp = getTemperatureAt(blob.position.y);

    // Update blob temperature (Newton's cooling)
    blob.update(dt, ambientTemp);

    // Calculate buoyancy based on temperature difference
    // At bottom (hot): blob temp ≈ ambient temp → density difference drives upward
    // At top (cold): blob temp ≈ ambient temp → density difference drives downward
    float tempDiff = blob.temperature - ambientTemp;

    // Buoyancy force proportional to temperature difference
    // Positive tempDiff (blob hotter than surroundings) → rise (negative y)
    // Negative tempDiff (blob cooler than surroundings) → fall (positive y)
    const float buoyancyStrength = 300.0f;
    float buoyancy = -tempDiff * buoyancyStrength;  // Negative because y increases downward

    // Apply forces: F = ma => a = F/m
    float acceleration = buoyancy / blob.mass;

    // Add quadratic drag for smoother deceleration (drag increases with velocity²)
    const float dragCoeff = 0.15f;  // Increased drag
    float drag = -dragCoeff * blob.velocity.y * std::abs(blob.velocity.y);

    // Update velocity with damping factor for smoother motion
    const float dampingFactor = 0.98f;  // Slight velocity reduction each frame
    blob.velocity.y = (blob.velocity.y + (acceleration + drag) * dt) * dampingFactor;

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
