#include "Molecule.h"
#include <cmath>


Molecule::Molecule(float x, float y, float initialTemp, float m)
    : position(x, y), velocity(0, 0), temperature(initialTemp), mass(m) {
    volume = BASE_VOLUME * (1.0f + THERMAL_EXPANSION_COEFF * temperature);
    radius = std::sqrt(volume) * 10.0f;
}

void Molecule::update(float dt, float ambientTemp) {
    // Newton cooling
    float tempDiff = temperature - ambientTemp;
    temperature -= COOLING_RATE * tempDiff * dt;

    // Extra push depending on zone (mild shaping)
    const float EXTRA_COOLING = 15.0f;
    if (ambientTemp < 40.0f)       temperature -= EXTRA_COOLING * dt; // colder near the top
    else if (ambientTemp > 80.0f)  temperature += EXTRA_COOLING * dt; // warmer near the bottom

    volume = BASE_VOLUME * (1.0f + THERMAL_EXPANSION_COEFF * temperature);
    radius = std::sqrt(volume) * 10.0f;
}

float Molecule::getDensity() const {
    // Density = mass / volume
    return mass / volume;
}

float Molecule::getBuoyancyForce(float mediumDensity) const {
    // Buoyancy force: F = (ρ_medium - ρ_molecule) * V * g
    const float g = 100.0f;
    return (mediumDensity - getDensity()) * volume * g;
}
