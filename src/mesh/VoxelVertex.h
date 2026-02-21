#pragma once
#include <glm.hpp>

struct VoxelVertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
    float layer;
};