#include "Presets.hpp"
#include <random>
#include <cmath>

std::size_t spawnNeutralPlasmaBatch(Simulator& sim, const PlasmaConfig& cfg, std::size_t alreadySpawned, std::size_t batch) {
    std::mt19937 rng(cfg.seed + (unsigned)alreadySpawned);
    std::uniform_real_distribution<float> unif(0.0f, 1.0f);
    std::normal_distribution<float> nrm(0.0f, cfg.thermalVsigma);

    std::size_t spawned = 0;
    std::size_t toSpawn = std::min(batch, cfg.N - alreadySpawned);
    for (std::size_t i = 0; i < toSpawn; ++i) {
        // sample uniformly in circle
        float r = std::sqrt(unif(rng)) * cfg.radius;
        float theta = 2.0f * 3.14159265358979323846f * unif(rng);
        float x = r * std::cos(theta) + sim.params().boundsW * 0.5f;
        float y = r * std::sin(theta) + sim.params().boundsH * 0.5f;
        float q = ((alreadySpawned + i) % 2 == 0) ? +cfg.qMag : -cfg.qMag;
        Particle p{{x,y}, {nrm(rng), nrm(rng)}, q, cfg.mass, cfg.particleRadius, (q>0? sf::Color::Red : sf::Color::Blue)};
        sim.addParticle(p);
        ++spawned;
    }
    return spawned;
}