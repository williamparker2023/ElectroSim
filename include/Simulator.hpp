#pragma once
#include <vector>
#include "Particle.hpp"

// Forward-declare to keep header light.
namespace sf {
    template <typename T> class Vector2;
    using Vector2f = Vector2<float>;
}

class Simulator {
public:
    // --------- Parameters (SI units) ----------
    struct Params {
        // Coulomb constant (N·m^2/C^2)
        float k = 8.9875517923e9f;

        // Softening distance^2 (m^2) to avoid singularities at r ~ 0
        float softening2 = 1e-4f; // (0.01 m)^2 default

        // Rectangular world bounds in meters (renderer maps m -> pixels)
        float boundsW = 8.0f;     // 8 m
        float boundsH = 6.0f;     // 6 m

        // Wall reflection coefficient (unitless). 1.0 = perfect reflection.
        float restitution = 1.0f;

        // Safety clamp for acceleration magnitude (m/s^2)
        float maxAccel = 1.0e2f;

        sf::Vector2f externalE{0.f, 0.f}; // NEW: uniform external E-field (V/m = N/C)
        float externalBz = 0.f; 
        // Stabilization / confinement (visual, tunable). Defaults = 0 so Sandbox is unaffected.
        float damping = 0.0f;             // s^-1 viscous damping (0 = off)
        float trapK  = 0.0f;              // N/m harmonic trap strength (0 = off)
        sf::Vector2f trapCenter{0.f, 0.f}; // meters
    };

    explicit Simulator(const Params& p);

    // World/particles
    void clear();
    void addParticle(const Particle& p);

    // Advance physics by dt seconds
    void step(float dt);

    // Accessors
    const std::vector<Particle>& particles() const { return particles_; }
          std::vector<Particle>& particles()       { return particles_; }

    const Params& params() const { return P; }
          Params& params()       { return P; }

    // Toggle electrostatics (default ON for you)
    void setElectrostaticsEnabled(bool on) { electroOn_ = on; }
    bool electrostaticsEnabled() const     { return electroOn_; }
    void setBoundsEnabled(bool on) { boundsOn_ = on; }
    bool boundsEnabled() const     { return boundsOn_; }

    void setExternalE(const sf::Vector2f& E) { P.externalE = E; }
    sf::Vector2f externalE() const { return P.externalE; }

    void  setExternalBz(float bz) { P.externalBz = bz; } 
    float externalBz() const      { return P.externalBz; }

private:
    Params P;
    std::vector<Particle> particles_;
    bool electroOn_ = true;

    // Internals — no collision handling between particles, only forces.
    void computeElectricField(std::vector<sf::Vector2f>& Eout) const; // O(N^2)   
    void applyBounds(); // bouncy walls only
    void integrateBoris(float dt, const std::vector<sf::Vector2f>& Efield);


    bool boundsOn_ = false; // default OFF
};