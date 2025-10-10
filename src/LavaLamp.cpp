#include "LavaLamp.h"
#include <algorithm>
#include <cmath>

LavaLamp::LavaLamp(int w, int h, int numMolecules)
    : width(w), height(h), hotTemp(100.0f), coldTemp(20.0f),
      mediumDensity(1.0f), spawnTimer(0.0f), rng(std::random_device{}()), xDist(50, w - 50) {
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
    if (molecules.size() >= MAX_MOLECULES) return;

    // Spawn at bottom with high temperature
    float x = xDist(rng);
    float y = height - 30.0f;
    float initialTemp = hotTemp + (rand() % 20 - 10);  // Random variation
    float mass = 0.5f + (rand() % 30) / 100.0f;        // Light mass: 0.5-0.8

    molecules.emplace_back(x, y, initialTemp, mass);
}

void LavaLamp::applyPhysics(Molecule& molecule, float dt) {
    // Get ambient temperature at molecule's position
    float ambientTemp = getTemperatureAt(molecule.position.y);

    // Update molecule temperature (Newton's cooling)
    molecule.update(dt, ambientTemp);

    // Calculate buoyancy based on temperature difference
    // At bottom (hot): molecule temp ≈ ambient temp → density difference drives upward
    // At top (cold): molecule temp ≈ ambient temp → density difference drives downward
    float tempDiff = molecule.temperature - ambientTemp;

    // Buoyancy force proportional to temperature difference
    // Positive tempDiff (molecule hotter than surroundings) → rise (negative y)
    // Negative tempDiff (molecule cooler than surroundings) → fall (positive y)
    const float buoyancyStrength = 300.0f;
    float buoyancy = -tempDiff * buoyancyStrength;  // Negative because y increases downward

    // Apply forces: F = ma => a = F/m
    float acceleration = buoyancy / molecule.mass;

    // Add quadratic drag for smoother deceleration (drag increases with velocity²)
    const float dragCoeff = 0.15f;  // Increased drag
    float drag = -dragCoeff * molecule.velocity.y * std::abs(molecule.velocity.y);

    // Update velocity with damping factor for smoother motion
    const float dampingFactor = 0.98f;  // Slight velocity reduction each frame
    molecule.velocity.y = (molecule.velocity.y + (acceleration + drag) * dt) * dampingFactor;

    // Limit velocity
    const float maxVelocity = 150.0f;
    if (molecule.velocity.y > maxVelocity) molecule.velocity.y = maxVelocity;
    if (molecule.velocity.y < -maxVelocity) molecule.velocity.y = -maxVelocity;

    // Update position
    molecule.position = molecule.position + molecule.velocity * dt;

    // Boundary checks - bounce off top/bottom
    if (molecule.position.y < molecule.radius) {
        molecule.position.y = molecule.radius;
        molecule.velocity.y *= -0.3f;  // Some damping
    }
    if (molecule.position.y > height - molecule.radius) {
        molecule.position.y = height - molecule.radius;
        molecule.velocity.y *= -0.3f;
    }
}

void LavaLamp::handleCollisions() {
    // Check all pairs of molecules for collision
    for (size_t i = 0; i < molecules.size(); ++i) {
        for (size_t j = i + 1; j < molecules.size(); ++j) {
            Molecule& b1 = molecules[i];
            Molecule& b2 = molecules[j];

            // Calculate distance between molecules
            float dx = b2.position.x - b1.position.x;
            float dy = b2.position.y - b1.position.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            float minDistance = b1.radius + b2.radius;

            // Check if molecules are colliding
            if (distance < minDistance && distance > 0.1f) {
                // Normalize collision vector
                float nx = dx / distance;
                float ny = dy / distance;

                // Relative velocity
                float dvx = b2.velocity.x - b1.velocity.x;
                float dvy = b2.velocity.y - b1.velocity.y;

                // Relative velocity in collision normal direction
                float dvn = dvx * nx + dvy * ny;

                // Do not resolve if velocities are separating
                if (dvn < 0) continue;

                // Calculate impulse scalar (elastic collision with restitution)
                const float restitution = 0.8f;  // Bounciness factor
                float impulse = -(1.0f + restitution) * dvn / (1.0f / b1.mass + 1.0f / b2.mass);

                // Apply impulse to velocities (conservation of momentum)
                b1.velocity.x -= impulse * nx / b1.mass;
                b1.velocity.y -= impulse * ny / b1.mass;
                b2.velocity.x += impulse * nx / b2.mass;
                b2.velocity.y += impulse * ny / b2.mass;

                // Separate overlapping molecules
                float overlap = minDistance - distance;
                float separationRatio = overlap / (b1.mass + b2.mass);
                b1.position.x -= nx * separationRatio * b2.mass;
                b1.position.y -= ny * separationRatio * b2.mass;
                b2.position.x += nx * separationRatio * b1.mass;
                b2.position.y += ny * separationRatio * b1.mass;

                // Exchange heat on collision (average temperatures)
                float avgTemp = (b1.temperature * b1.mass + b2.temperature * b2.mass) / (b1.mass + b2.mass);
                float tempExchange = 0.3f;  // 30% heat exchange
                b1.temperature += (avgTemp - b1.temperature) * tempExchange;
                b2.temperature += (avgTemp - b2.temperature) * tempExchange;
            }
        }
    }
}

void LavaLamp::update(float dt) {
    // Spawn new molecules periodically
    spawnTimer += dt;
    if (spawnTimer >= SPAWN_INTERVAL) {
        spawnMolecule();
        spawnTimer = 0.0f;
    }

    // Update all molecules
    for (auto& molecule : molecules) {
        applyPhysics(molecule, dt);
    }

    // Handle collisions between molecules
    handleCollisions();

    // Remove molecules that are too old or stable
    molecules.erase(
        std::remove_if(molecules.begin(), molecules.end(), [this](const Molecule& b) {
            // Remove if outside bounds significantly
            return b.position.y < -100 || b.position.y > height + 100;
        }),
        molecules.end()
    );
}
