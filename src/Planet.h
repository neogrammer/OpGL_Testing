#pragma once
#include <glm.hpp>
#include "Voxel.h"
#include "Noise.h"
#include <cmath>

struct PlanetParams {
    float baseRadius = 64.0f;  // voxels
    float maxHeight = 12.0f;
    float noiseFreq = 3.0f;
    int   octaves = 5;
};

inline float HeightOnSphere(glm::vec3 dir, const PlanetParams& pp) {
    // dir normalized
    float h = FBM(dir * pp.noiseFreq, pp.octaves); // [-1,1]
    return h * pp.maxHeight;
}

inline Block SamplePlanet(glm::vec3 p, const PlanetParams& pp) {
    float d = glm::length(p);
    if (d < 1e-5f) return Block::Stone;

    glm::vec3 dir = p / d;
    float surfaceR = pp.baseRadius + HeightOnSphere(dir, pp);
    float depth = surfaceR - d; // >0 inside

    if (depth <= 0.0f) return Block::Air;

    // top layer choice (simple biome)
    if (depth < 1.0f) {
        float lat = dir.y; // [-1,1]
        bool polar = std::abs(lat) > 0.65f;
        bool high = (surfaceR - pp.baseRadius) > pp.maxHeight * 0.35f;
        if (polar || high) return Block::Snow;
        return Block::Grass;
    }
    if (depth < 4.0f) return Block::Dirt;
    return Block::Stone;
}