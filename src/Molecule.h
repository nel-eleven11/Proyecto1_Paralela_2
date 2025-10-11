#ifndef MOLECULE_H
#define MOLECULE_H

#include <cmath>

struct Vec2 {
    float x, y;
    Vec2(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
};

class Molecule {
public:
    Vec2  position;
    Vec2  velocity;
    float temperature;   // Current temperature in °C
    float volume;        // Arbitrary units
    float mass;          // Mass (constant per molecule)
    float radius;        // Visual radius
    int   blobId = -1;   // Cluster membership (-1 = unassigned)

    Molecule(float x, float y, float initialTemp, float m);

    // Update molecule physics (cooling + thermal expansion)
    void  update(float dt, float ambientTemp);

    // Density = mass / volume
    float getDensity() const;

    // Buoyancy force = (rho_medium - rho_blob) * V * g
    float getBuoyancyForce(float mediumDensity) const;

private:
    static constexpr float BASE_VOLUME = 0.35f;   // smaller than the old Blob (to have more molecules)
    static constexpr float THERMAL_EXPANSION_COEFF = 0.02f;
    static constexpr float COOLING_RATE = 2.0f;
};

#endif
