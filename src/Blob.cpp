#include "Blob.h"
#include <cmath>

Blob::Blob(float x, float y, float initialTemp, float m)
    : position(x, y), velocity(0, 0), temperature(initialTemp), mass(m) {
    // Initial volume based on temperature
    volume = BASE_VOLUME * (1.0f + THERMAL_EXPANSION_COEFF * temperature);
    radius = std::sqrt(volume) * 10.0f;  // Visual radius
}

void Blob::update(float dt, float ambientTemp) {
    // Newton's Law of Cooling: dT/dt = -k(T - T_ambient)
    float tempDiff = temperature - ambientTemp;
    temperature -= COOLING_RATE * tempDiff * dt;

    // Update volume based on temperature (thermal expansion)
    // V = V0 * (1 + α * ΔT)
    volume = BASE_VOLUME * (1.0f + THERMAL_EXPANSION_COEFF * temperature);
    radius = std::sqrt(volume) * 10.0f;
}

float Blob::getDensity() const {
    // Density = mass / volume
    return mass / volume;
}

float Blob::getBuoyancyForce(float mediumDensity) const {
    // Buoyancy force: F = (ρ_medium - ρ_blob) * V * g
    // We'll use simplified gravity constant
    const float g = 100.0f;
    return (mediumDensity - getDensity()) * volume * g;
}
