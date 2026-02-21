#include "World.h"
#include "Mesher.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <algorithm>
#include <iostream>

void World::BuildPlanetOnce() {
    // Build a whole small planet in a bounded chunk cube.
    float rMax = planet.baseRadius + planet.maxHeight + 2.0f;
    int minW = (int)std::floor(-rMax);
    int maxW = (int)std::ceil(rMax);

    int minC = FloorDiv(minW, CHUNK_SIZE);
    int maxC = FloorDiv(maxW, CHUNK_SIZE);

    // 1) create + fill chunks
    for (int cz = minC; cz <= maxC; cz++)
        for (int cy = minC; cy <= maxC; cy++)
            for (int cx = minC; cx <= maxC; cx++) {
                Chunk& c = GetOrCreateChunk({ cx,cy,cz });
                FillChunkBlocks(c);
            }

    // 2) build meshes
    RebuildDirtyMeshes();
}

void World::RebuildDirtyMeshes() {
    for (auto& [cc, c] : chunks) {
        if (c.dirty) BuildChunkMesh(c);
    }
}