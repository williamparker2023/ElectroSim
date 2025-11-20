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

void Simulator::computeElectricField(std::vector<sf::Vector2f>& Eout) const {
    const size_t n = particles_.size();
    Eout.assign(n, P.externalE); // start with uniform external field

    if (!electroOn_ || n == 0) return;

    const float k = P.k;
    // symmetric O(N^2) pair contributions
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const auto rij = particles_[i].pos - particles_[j].pos; // meters
            float r2 = rij.x*rij.x + rij.y*rij.y + P.softening2;
            float invR  = 1.0f / std::sqrt(r2);
            float invR3 = invR * invR * invR;

            // E contribution from particle j at i: k * q_j * (r_i - r_j) / |r|^3
            sf::Vector2f Ej = { k * particles_[j].charge * rij.x * invR3,
                                k * particles_[j].charge * rij.y * invR3 };
            // E contribution from particle i at j is opposite direction with q_i
            sf::Vector2f Ei = { -k * particles_[i].charge * rij.x * invR3,
                                -k * particles_[i].charge * rij.y * invR3 };

            Eout[i] += Ej;
            Eout[j] += Ei;
        }
    }
}


void Simulator::integrateBoris(float dt, const std::vector<sf::Vector2f>& Efield) {
    const float Bz = P.externalBz;
    const size_t n = particles_.size();
    for (size_t i = 0; i < n; ++i) {
        Particle &p = particles_[i];
        double q = (double)p.charge;
        double m = (double)p.mass;
        double dt2 = 0.5 * (double)dt;

        // E half-kick
        double vx = p.vel.x + (q/m) * (double)Efield[i].x * dt2;
        double vy = p.vel.y + (q/m) * (double)Efield[i].y * dt2;

        // Magnetic rotation (B only z)
        double tz = (q/m) * (double)Bz * dt2;
        // rotate velocity (standard 2D Boris algebra)
        double vpx = vx +  vy * tz;
        double vpy = vy -  vx * tz;
        double s = 2.0 * tz / (1.0 + tz*tz);
        double vfx = vx +  vpy * s;
        double vfy = vy -  vpx * s;

        // E half-kick
        vfx += (q/m) * (double)Efield[i].x * dt2;
        vfy += (q/m) * (double)Efield[i].y * dt2;

        // commit (convert back to float)
        p.vel.x = (float)vfx;
        p.vel.y = (float)vfy;

        // position update
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
}

void Simulator::step(float dt) {
    std::vector<sf::Vector2f> Efield(particles_.size());
    computeElectricField(Efield);
    integrateBoris(dt, Efield);
    if (boundsOn_) applyBounds();
}