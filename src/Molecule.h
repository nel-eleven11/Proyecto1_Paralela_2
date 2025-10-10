#ifndef MOLECULE_H
#define MOLECULE_H

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
};

class Molecule {
public:
    Vec2 position;
    Vec2 velocity;
    float temperature;      // Current temperature in C
    float volume;           // Volume in arbitrary units
    float mass;             // Mass (constant)
    float radius;           // Visual radius
    int   blobId = -1;      // cluster membership

    Molecule(float x, float y, float initialTemp, float mass);

    // Update molecule physics
    void update(float dt, float ambientTemp);

    // Calculate density based on temperature and mass
    float getDensity() const;

    // Calculate buoyancy force based on density difference with medium
    float getBuoyancyForce(float mediumDensity) const;

private:
    static constexpr float BASE_VOLUME = 0.35f;
    static constexpr float THERMAL_EXPANSION_COEFF = 0.02f;   // Volume expansion per °C
    static constexpr float COOLING_RATE = 2.0f;               // Newton's cooling coefficient (smooth transitions)
};

#endif
