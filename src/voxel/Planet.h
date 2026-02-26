#pragma once
#include <glm.hpp>
#include "Voxel.h"
#include "Noise.h"
#include <cmath>

struct PlanetParams {
    float baseRadius = 1024.0f;  // voxels
    float maxHeight = 512.0f;
    float noiseFreq = 16.0f;
    int   octaves = 16;

    float seaLevelOffset = -2.f;
};

inline float HeightOnSphere(glm::vec3 dir, const PlanetParams& pp) {
    // dir normalized
    float h = FBM(dir * pp.noiseFreq, pp.octaves); // [-1,1]
    return h * pp.maxHeight;
}

// Inputs 
//  p = world position(voxel center), in voxel units
//  depth = (surfaceR - d).depth > 0 means inside planet

// Output:
//  true -> carve to air
inline bool ShouldCarveCave(glm::vec3 p, float depth) {
    if (depth < 4.0f) return false;
    float n = FBM(p * 0.06f, 4);
    return n > 0.35f;
}

inline bool ShoudCarveCave(glm::vec3 p, float depth) {
    if (depth < 4.0f) return false;
    float n = FBM(p * 0.06f, 4);
    return n > 0.35f;
}

inline Block SamplePlanet(glm::vec3 p, const PlanetParams& pp) {
    float d = glm::length(p);
    if (d < 1e-5f) return Block::Stone;

    glm::vec3 dir = p / d;
    float surfaceR = pp.baseRadius + HeightOnSphere(dir, pp);
    float depth = surfaceR - d; // >0 inside

    if (depth <= 0.0f) return Block::Air;

    // Choose base solid material
    Block solid;
    if (depth < 1.0f) solid = Block::Grass;
    else if (depth < 4.f) solid = Block::Dirt;
    else solid = Block::Stone;

    // carve caves
    if (ShouldCarveCave(p, depth)) return Block::Air;

    return solid;

    //// top layer choice (simple biome)
    //if (depth < 1.0f) {
    //    float lat = dir.y; // [-1,1]
    //    bool polar = std::abs(lat) > 0.65f;
    //    bool high = (surfaceR - pp.baseRadius) > pp.maxHeight * 0.35f;
    //    if (polar || high) return Block::Snow;
    //    return Block::Grass;
    //}
    //if (depth < 4.0f) return Block::Dirt;
    //return Block::Stone;
}
//
//inline bool ShouldRenderFace(Block self, Block neighbor) {
//    if (self == Block::Water) {
//        return neighbor == Block::Air || IsOpaque(neighbor);
//    }
//    if (IsOpaque(self)) {
//        return !IsOpaque(neighbor);  //neighbor is air or water
//    }
//    return false;  // Air has no faces
//}

inline Block SamplePlanetWithOcean(glm::vec3 p, const PlanetParams& pp)
{
    float d = glm::length(p);
    if (d < 1e-5f) return Block::Stone;

    glm::vec3 dir = p / d;

    float height = HeightOnSphere(dir, pp);
    float surfaceR = pp.baseRadius + height;
    float seaR = pp.baseRadius + pp.seaLevelOffset;

    // 1) Outside the solid surface: air or water
    if (d > surfaceR) {
        if (d < seaR) return Block::Water; // ocean fills low areas
        return Block::Air;
    }

    // 2) Inside the planet: choose material by depth
    float depth = surfaceR - d; // >0 inside

    // caves
    if (ShouldCarveCave(p, depth)) return Block::Air;

    // top layer (beach if below/near sea)
    if (depth < 1.0f) {
        if (surfaceR < seaR + 0.5f) return Block::Sand;

        // optional snow biome
        float lat = dir.y; // [-1,1]
        bool polar = std::abs(lat) > 0.65f;
        bool high = height > pp.maxHeight * 0.35f;
        if (polar || high) return Block::Snow;

        return Block::Grass;
    }

    if (depth < 4.0f) return Block::Dirt;
    return Block::Stone;
}



