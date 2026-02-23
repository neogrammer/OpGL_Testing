#pragma once
#include <array>
#include <vector>
#include <glad/glad.h>
#include <glm.hpp>
#include "Voxel.h"
#include "../app/GpuMesh.h"

static constexpr int CHUNK_SIZE = 16;

struct ChunkCoord { int x, y, z; };

struct Chunk {
    ChunkCoord coord{};
    std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE> blocks{};

    //GLuint vao = 0;
    //GLuint vbo = 0;
    //int vertexCount = 0;
    GpuMesh opaque;
    GpuMesh water;

    bool dirty = true;
    bool generated = false;
    bool queuedGen = false;
    bool queuedMesh = false;
};

inline int Idx(int x, int y, int z) {
    return x + CHUNK_SIZE * (y + CHUNK_SIZE * z);
}