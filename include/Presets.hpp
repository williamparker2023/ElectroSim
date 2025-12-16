#pragma once
#include <cstddef>
#include "Simulator.hpp"

struct PlasmaConfig {
    std::size_t N = 10000;
    float radius = 0.5f;
    float qMag  = 1e-7f;
    float mass  = 1e-4f;
    float particleRadius = 0.003f;
    float thermalVsigma = 0.1f;
    unsigned seed = 123456;
    // coherent rotation (rad/s) applied as v_theta = omega * r
    float omega = 0.5f;
};

/// Spawn up to 'batch' particles into sim according to cfg. Returns how many were spawned.
std::size_t spawnNeutralPlasmaBatch(Simulator& sim, const PlasmaConfig& cfg, std::size_t alreadySpawned, std::size_t batch);