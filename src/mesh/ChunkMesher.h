#pragma once
#include <array>
#include <functional>
#include <vector>
#include <glm.hpp>

#include <../voxel/Chunk.h>
#include <../voxel/Voxel.h>
#include "VoxelVertex.h"

struct ChunkMeshData {
    std::vector<VoxelVertex> opaque;
    std::vector<VoxelVertex> water;
};

using GetBlockFn = std::function<Block(int, int, int)>;

ChunkMeshData BuildChunkMeshFaceCulled(
    const std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>& blocks,
    const glm::ivec3& chunkBase,
    const GetBlockFn& getBlockWorld,
    int cubeNetW, int cubeNetH);

ChunkMeshData BuildChunkMeshGreedy(
    const std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>& blocks,
    const glm::ivec3& chunkBase,
    const GetBlockFn& getBlockWorld,
    int cubeNetW, int cubeNetH);