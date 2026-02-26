#pragma once


#include <unordered_map>
#include <vector>
#include <deque>
#include <glm.hpp>
#include "Chunk.h"
#include "Planet.h"
#include <algorithm>
#include <cstdlib>

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
    struct StreamStats
    {
        size_t loaded = 0;
        size_t generated = 0;
        size_t meshed = 0;
        size_t target = 0;
        size_t genQ = 0;
        size_t meshQ = 0;
        int renderDistance = 0;
        int unloadDistance = 0;
    };

    int GetRenderDistance() const { return renderDistance; }
    int GetUnloadDistance() const { return unloadDistance; }

    StreamStats GetStreamStats() const
    {
        StreamStats s;
        s.loaded = chunks.size();
        s.genQ = genQueue.size();
        s.meshQ = meshQueue.size();
        s.renderDistance = renderDistance;
        s.unloadDistance = unloadDistance;

        int side = 2 * renderDistance + 1;
        s.target = (size_t)side * (size_t)side * (size_t)side;

        for (auto& [coord, c] : chunks)
        {
            if (c.generated) s.generated++;
            if (c.generated && !c.dirty) s.meshed++;
        }
        return s;
    }

    // Strict “ready”: every chunk in the render cube exists + generated + meshed
    bool IsStreamReady() const
    {
        ChunkCoord cc = streamCamChunk;

        for (int dz = -renderDistance; dz <= renderDistance; ++dz)
            for (int dy = -renderDistance; dy <= renderDistance; ++dy)
                for (int dx = -renderDistance; dx <= renderDistance; ++dx)
                {
                    ChunkCoord want{ cc.x + dx, cc.y + dy, cc.z + dz };
                    auto it = chunks.find(want);
                    if (it == chunks.end()) return false;

                    const Chunk& c = it->second;
                    if (!c.generated) return false;
                    if (c.dirty) return false;
                }
        return true;
    }
 


    Chunk& GetOrCreateChunk(ChunkCoord cc);
    Block GetBlock(int wx, int wy, int wz) const;

    void BuildPlanetOnce();        // simple start: generate whole small planet
    void RebuildDirtyMeshes();     // mesh upload step

    //void Draw() const;
    //inline void DrawOpaque() const {
    //    for (auto& [cc, c] : chunks) { c.opaque.Draw(); }
    //    glBindVertexArray(0);

    //}

    inline void DrawOpaque() const {
        for (auto& [cc, c] : chunks)
        {
            int ddx = cc.x - streamCamChunk.x;
            int ddy = cc.y - streamCamChunk.y;
            int ddz = cc.z - streamCamChunk.z;
            int distCheby = std::max({ std::abs(ddx), std::abs(ddy), std::abs(ddz) });

            if (distCheby <= renderDistance)
                c.opaque.Draw();
        }
        glBindVertexArray(0);
    }



     //inline void DrawWater() const {
     //    for (auto& [cc, c] : chunks) c.water.Draw();
     //    glBindVertexArray(0);
     //}

    inline void DrawWater() const {
        for (auto& [cc, c] : chunks)
        {
            int ddx = cc.x - streamCamChunk.x;
            int ddy = cc.y - streamCamChunk.y;
            int ddz = cc.z - streamCamChunk.z;
            int distCheby = std::max({ std::abs(ddx), std::abs(ddy), std::abs(ddz) });

            if (distCheby <= renderDistance)
                c.water.Draw();
        }
        glBindVertexArray(0);
    }

    void UpdateStreaming(glm::vec3 cameraPos, glm::vec3 cameraForward);
    void TickBuildQueues(int maxGenPerFrame, int maxMeshPerFrame);
    void DrawWaterSorted(const glm::vec3& cameraPos);

    void SetRenderDistance(int d) { renderDistance = d; }
    void SetLoadDistance(int d) { loadDistance = d; }
    void SetUnloadDistance(int d) { unloadDistance = d; }

    int GetLoadDistance()   const { return loadDistance; }


    // Streaming parameters (handy for fog / UI / debug)
    
    

private:
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;

    std::deque<ChunkCoord> genQueue;
    std::deque<ChunkCoord> meshQueue;

    int renderDistance = 7;
    int loadDistance = renderDistance; // streaming/build distance
    int unloadDistance = 8;



    ChunkCoord streamCamChunk{ 0,0,0 };
    glm::vec3  streamCamForward{ 0,0,-1 }; // normalized
    float      streamFrontBias = 8.0f;     // bigger = more “front-first”

    void FillChunkBlocks(Chunk& c);
    void BuildChunkMesh(Chunk& c);

    int cubeNetW = 128;
    int cubeNetH = 96;
};

