#include "Presets.hpp"
#include "Particle.hpp"
#include <random>
#include <cmath>

std::size_t spawnNeutralPlasmaBatch(Simulator& sim, const PlasmaConfig& cfg, std::size_t alreadySpawned, std::size_t batch) {
    if (cfg.N == 0) return 0;
    std::size_t remaining = (cfg.N > alreadySpawned) ? (cfg.N - alreadySpawned) : 0;
    std::size_t toSpawn = std::min(batch, remaining);
    if (toSpawn == 0) return 0;

    // deterministic RNG per batch offset so repeated runs are reproducible
    std::mt19937 rng(cfg.seed + static_cast<unsigned>(alreadySpawned));
    std::uniform_real_distribution<float> unif(0.0f, 1.0f);
    std::normal_distribution<float> nrm(0.0f, cfg.thermalVsigma);

    // center for the cloud: prefer sim params trapCenter if set, otherwise world center
    const auto& prm = sim.params();
    sf::Vector2f center = prm.trapCenter;
    if (center.x == 0.f && center.y == 0.f) {
        center = { prm.boundsW * 0.5f, prm.boundsH * 0.5f };
    }

    std::size_t spawned = 0;
    for (std::size_t i = 0; i < toSpawn; ++i) {
        std::size_t globalIdx = alreadySpawned + i;
        // sample uniformly in circle
        float u = unif(rng);
        float r = std::sqrt(u) * cfg.radius;
        float theta = 2.0f * 3.14159265358979323846f * unif(rng);
        float cx = center.x + r * std::cos(theta);
        float cy = center.y + r * std::sin(theta);

        // balanced sign: alternate signs for near-exact neutrality
        float q = ((globalIdx & 1u) == 0u) ? +cfg.qMag : -cfg.qMag;

        // coherent azimuthal velocity + small thermal noise
        float v_theta = cfg.omega * r;
        float vx = -std::sin(theta) * v_theta + nrm(rng);
        float vy =  std::cos(theta) * v_theta + nrm(rng);

        sf::Color col = (q > 0.0f) ? sf::Color::Red : sf::Color::Blue;

        Particle p;
        p.pos = { cx, cy };
        p.vel = { vx, vy };
        p.charge = q;
        p.mass = cfg.mass;
        p.radius = cfg.particleRadius;
        p.color = col;

        sim.addParticle(p);
        ++spawned;
    }

    return spawned;
}