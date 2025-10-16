#include "Simulator.hpp"
#include <SFML/System/Vector2.hpp> // sf::Vector2f
#include <algorithm>
#include <cmath>

static inline sf::Vector2f clampMag(const sf::Vector2f& v, float maxMag) {
    float m2 = v.x*v.x + v.y*v.y;
    if (m2 <= maxMag*maxMag) return v;
    float inv = maxMag / std::sqrt(m2);
    return {v.x * inv, v.y * inv};
}

Simulator::Simulator(const Params& p) : P(p) {}

void Simulator::clear() { particles_.clear(); }

void Simulator::addParticle(const Particle& p) { particles_.push_back(p); }

// O(N^2) Coulomb forces with softening; no particle collisions.
void Simulator::computeForcesNaive(std::vector<sf::Vector2f>& acc) const {
    const size_t n = particles_.size();
    std::fill(acc.begin(), acc.end(), sf::Vector2f{0.f, 0.f});

    if (!electroOn_ || n==0) return;

    for (size_t i=0; i<n; ++i){
        for(size_t j=i+1; j<n; ++j){
            const auto rij = particles_[i].pos - particles_[j].pos; // meters
            float r2 = rij.x*rij.x + rij.y*rij.y + P.softening2;    // m^2 + ε^2
            float invR  = 1.0f / std::sqrt(r2);
            float invR3 = invR * invR * invR;
            
            // Coulomb force magnitude scaled into vector form (N/m * m = N)
            const float s = P.k * (particles_[i].charge * particles_[j].charge) * invR3;\
            sf::Vector2f f = { s * rij.x, s * rij.y }; // Newtons

            // Accelerations (m/s^2)
            sf::Vector2f ai = { f.x / particles_[i].mass, f.y / particles_[i].mass };
            sf::Vector2f aj = { -f.x / particles_[j].mass, -f.y / particles_[j].mass };

            acc[i] += ai;
            acc[j] += aj;
        }
    }
    for (auto& a : acc) a = clampMag(a, P.maxAccel); // numerical safety
}

// Symplectic Euler: v_{t+dt} = v_t + a_t dt ; x_{t+dt} = x_t + v_{t+dt} dt
void Simulator::integrateSymplecticEuler(float dt, std::vector<sf::Vector2f>& acc) {
    const size_t n = particles_.size();
    for (size_t i = 0; i < n; ++i) {
        particles_[i].vel += acc[i] * dt;            // m/s
        particles_[i].pos += particles_[i].vel * dt; // m
    }
}

// Reflect from rectangular bounds (meters)
void Simulator::applyBounds() {
    for (auto& p : particles_) {
        // Left/Right
        if (p.pos.x < p.radius) {
            p.pos.x = p.radius;
            p.vel.x = -p.vel.x * P.restitution;
        } else if (p.pos.x > P.boundsW - p.radius) {
            p.pos.x = P.boundsW - p.radius;
            p.vel.x = -p.vel.x * P.restitution;
        }
        // Top/Bottom
        if (p.pos.y < p.radius) {
            p.pos.y = p.radius;
            p.vel.y = -p.vel.y * P.restitution;
        } else if (p.pos.y > P.boundsH - p.radius) {
            p.pos.y = P.boundsH - p.radius;
            p.vel.y = -p.vel.y * P.restitution;
        }
    }
}

void Simulator::step(float dt) {
    std::vector<sf::Vector2f> acc(particles_.size());
    computeForcesNaive(acc);
    integrateSymplecticEuler(dt, acc);
    applyBounds();
}