#pragma once


#include <unordered_map>
#include <vector>
#include <deque>
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

inline ChunkCoord WorldToChunk(int wx, int wy, int wz) {
    return {
        FloorDiv(wx, CHUNK_SIZE),
        FloorDiv(wy, CHUNK_SIZE),
        FloorDiv(wz, CHUNK_SIZE)
    };
}

extern glm::ivec3 CameraVoxel(glm::vec3 camPos);

extern ChunkCoord CameraChunk(glm::vec3 camPos);

static const glm::ivec2 TILE_TOP = { 1,2 };
static const glm::ivec2 TILE_BOTTOM = { 1,0 };

// what tile to use when a face is acting like a “side”
// (for +Y/-Y acting as sides, we just use the front side tile)
static const glm::ivec2 SIDE_TILE[6] = {
    {2,1}, // +X  -> right side tile
    {0,1}, // -X  -> left side tile
    {1,1}, // +Y  -> use front side tile
    {1,1}, // -Y  -> use front side tile
    {1,1}, // +Z  -> front side tile
    {3,1}, // -Z  -> back side tile
};



class World {
public:
    PlanetParams planet;

 


    Chunk& GetOrCreateChunk(ChunkCoord cc);
    Block GetBlock(int wx, int wy, int wz) const;

    void BuildPlanetOnce();        // simple start: generate whole small planet
    void RebuildDirtyMeshes();     // mesh upload step

    //void Draw() const;
    inline void DrawOpaque() const {
        for (auto& [cc, c] : chunks) { c.opaque.Draw(); }
        glBindVertexArray(0);

    }



     inline void DrawWater() const {
         for (auto& [cc, c] : chunks) c.water.Draw();
         glBindVertexArray(0);
     }

    void UpdateStreaming(glm::vec3 cameraPos);
    void TickBuildQueues(int maxGenPerFrame, int maxMeshPerFrame);

private:
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;

    std::deque<ChunkCoord> genQueue;
    std::deque<ChunkCoord> meshQueue;

    int renderDistance = 2;
    int unloadDistance = 8;

    void FillChunkBlocks(Chunk& c);
    void BuildChunkMesh(Chunk& c);

    int cubeNetW = 128;
    int cubeNetH = 96;
};

