#pragma once
#include <cstddef>
#include "Simulator.hpp"

struct PlasmaConfig {
    std::size_t N = 10000;        // total particles to create
    float radius = 0.5f;         // meters
    float qMag  = 1e-7f;         // Coulomb per macro-particle
    float mass  = 1e-4f;         // kg
    float particleRadius = 0.003f; // m (visual)
    float thermalVsigma = 0.1f;  // m/s
    unsigned seed = 123456;
};

/// Spawn up to 'batch' particles into sim according to cfg. Returns how many were spawned.
std::size_t spawnNeutralPlasmaBatch(Simulator& sim, const PlasmaConfig& cfg, std::size_t alreadySpawned, std::size_t batch);