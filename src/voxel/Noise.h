#pragma once
#include <glm.hpp>
#include <cstdint>

inline uint32_t HashU32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352d;
    x ^= x >> 15; x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

inline float Hash3i(int x, int y, int z) {
    uint32_t h = 2166136261u;
    h = (h ^ (uint32_t)x) * 16777619u;
    h = (h ^ (uint32_t)y) * 16777619u;
    h = (h ^ (uint32_t)z) * 16777619u;
    h = HashU32(h);
    return (h & 0x00FFFFFFu) / float(0x01000000u); // [0,1)
}

inline float Smooth(float t) { return t * t * (3.0f - 2.0f * t); }

inline float ValueNoise3(glm::vec3 p) {
    glm::ivec3 i = glm::floor(p);
    glm::vec3  f = p - glm::vec3(i);

    float sx = Smooth(f.x), sy = Smooth(f.y), sz = Smooth(f.z);

    auto R = [&](int dx, int dy, int dz) {
        return Hash3i(i.x + dx, i.y + dy, i.z + dz);
        };

    float c000 = R(0, 0, 0), c100 = R(1, 0, 0), c010 = R(0, 1, 0), c110 = R(1, 1, 0);
    float c001 = R(0, 0, 1), c101 = R(1, 0, 1), c011 = R(0, 1, 1), c111 = R(1, 1, 1);

    float x00 = glm::mix(c000, c100, sx);
    float x10 = glm::mix(c010, c110, sx);
    float x01 = glm::mix(c001, c101, sx);
    float x11 = glm::mix(c011, c111, sx);

    float y00 = glm::mix(x00, x10, sy);
    float y01 = glm::mix(x01, x11, sy);

    return glm::mix(y00, y01, sz); // [0,1)
}

inline float FBM(glm::vec3 p, int octaves) {
    float sum = 0.0f;
    float amp = 0.5f;
    float freq = 1.0f;
    for (int i = 0; i < octaves; i++) {
        float n = ValueNoise3(p * freq) * 2.0f - 1.0f; // [-1,1]
        sum += n * amp;
        freq *= 2.0f;
        amp *= 0.5f;
    }
    return sum; // roughly [-1,1]
}