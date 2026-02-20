#pragma once
#include <unordered_map>
#include <vector>
#include <glm.hpp>
#include "Chunk.h"
#include "Planet.h"

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const noexcept {
        size_t h1 = std::hash<int>{}(c.x);
        size_t h2 = std::hash<int>{}(c.y);
        size_t h3 = std::hash<int>{}(c.z);
        return h1 ^ (h2 * 0x9e3779b97f4a7c15ull) ^ (h3 * 0x85ebca6bull);
    }
};

inline bool operator==(const ChunkCoord& a, const ChunkCoord& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

// handles negatives correctly
inline int FloorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) q--;
    return q;
}
inline int Mod(int a, int b) {
    int m = a % b;
    if (m < 0) m += (b < 0 ? -b : b);
    return m;
}

class World {
public:
    PlanetParams planet;

    Chunk& GetOrCreateChunk(ChunkCoord cc);
    Block GetBlock(int wx, int wy, int wz) const;

    void BuildPlanetOnce();        // simple start: generate whole small planet
    void RebuildDirtyMeshes();     // mesh upload step

    void Draw() const;

private:
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;

    void FillChunkBlocks(Chunk& c);
    void BuildChunkMesh(Chunk& c);
};