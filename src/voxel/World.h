#pragma message("USING World.h from: " __FILE__)

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





class World {
public:
    PlanetParams planet;



    Chunk& GetOrCreateChunk(ChunkCoord cc);
    Block GetBlock(int wx, int wy, int wz) const;

    void BuildPlanetOnce();        // simple start: generate whole small planet
    void RebuildDirtyMeshes();     // mesh upload step

    //void Draw() const;
    void DrawOpaque() const;
    void DrawWater() const;

    void UpdateStreaming(glm::vec3 cameraPos);
    void TickBuildQueues(int maxGenPerFrame, int maxMeshPerFrame);

private:
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;

    std::deque<ChunkCoord> genQueue;
    std::deque<ChunkCoord> meshQueue;

    int renderDistance = 1;
    int unloadDistance = 2;

    void FillChunkBlocks(Chunk& c);
    void BuildChunkMesh(Chunk& c);

    int cubeNetW = 128;
    int cubeNetH = 96;
};