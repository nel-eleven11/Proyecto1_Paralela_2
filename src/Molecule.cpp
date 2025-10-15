#include "Molecule.h"
#include <cmath>

Molecule::Molecule(float x, float y, float initialTemp, float m)
    : position(x, y), velocity(0, 0), temperature(initialTemp), mass(m) {
    volume = BASE_VOLUME * (1.0f + THERMAL_EXPANSION_COEFF * temperature);
    radius = std::sqrt(volume) * 8.0f;
}

void Molecule::update(float dt, float ambientTemp) {
    float tempDiff = temperature - ambientTemp;
    temperature -= COOLING_RATE * tempDiff * dt;

    const float EXTRA_COOLING = 15.0f;
    if (ambientTemp < 40.0f)       temperature -= EXTRA_COOLING * dt;
    else if (ambientTemp > 80.0f)  temperature += EXTRA_COOLING * dt;

    volume = BASE_VOLUME * (1.0f + THERMAL_EXPANSION_COEFF * temperature);
    radius = std::sqrt(volume) * 8.0f;
}

float Molecule::getDensity() const {
    return mass / volume;
}

float Molecule::getBuoyancyForce(float mediumDensity) const {
    const float g = 100.0f;
    return (mediumDensity - getDensity()) * volume * g;
}
