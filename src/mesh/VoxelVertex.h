#pragma once
#include <glm.hpp>

struct VoxelVertex {
    glm::vec3 pos;
    glm::vec2 localUV;   // replaces uv
    glm::vec3 normal;
    float layer;
    glm::vec2 tile;      // new: (col,row)
};