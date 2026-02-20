#pragma once
#include <array>
#include <vector>
#include <glad/glad.h>
#include <glm.hpp>
#include "Voxel.h"

static constexpr int CHUNK_SIZE = 16;

struct ChunkCoord { int x, y, z; };

struct Chunk {
    ChunkCoord coord{};
    std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE> blocks{};

    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;

    bool dirty = true;
};

inline int Idx(int x, int y, int z) {
    return x + CHUNK_SIZE * (y + CHUNK_SIZE * z);
}