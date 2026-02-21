#pragma once
#include <vector>
#include <glad/glad.h>
#include "../mesh/VoxelVertex.h"

struct GpuMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    int count = 0;

    void Upload(const std::vector<VoxelVertex>& verts);
    void Draw() const;
    void Destroy();
};