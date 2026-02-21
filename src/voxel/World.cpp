//#include "World.h"
//#include "Mesher.h"
//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//#include <glm.hpp>
//#include <algorithm>
//#include <iostream>
//
//struct VoxelVertex { glm::vec3 pos; glm::vec2 uv; glm::vec3 normal; float layer; };
//static const glm::ivec2 TILE_TOP = { 1,2 };
//static const glm::ivec2 TILE_BOTTOM = { 1,0 };
//
//// what tile to use when a face is acting like a “side”
//// (for +Y/-Y acting as sides, we just use the front side tile)
//static const glm::ivec2 SIDE_TILE[6] = {
//    {2,1}, // +X  -> right side tile
//    {0,1}, // -X  -> left side tile
//    {1,1}, // +Y  -> use front side tile
//    {1,1}, // -Y  -> use front side tile
//    {1,1}, // +Z  -> front side tile
//    {3,1}, // -Z  -> back side tile
//};
//
//// Map from "canonical quad corners" Q[0..3] (made from (i,j,w,h) in u/v order)
//// into the corner order expected by FACES[faceIndex].c[0..3] + GetCubeNetUVsTile.
//static const uint8_t FACE_Q_MAP[6][4] = {
//    {0,1,2,3}, // +X
//    {3,2,1,0}, // -X
//    {1,2,3,0}, // +Y
//    {0,3,2,1}, // -Y
//    {1,2,3,0}, // +Z
//    {0,3,2,1}, // -Z
//};
//
//static inline int DominantTopFaceIndex(const glm::ivec3& voxelWorld) {
//    glm::vec3 center = glm::vec3(voxelWorld) + glm::vec3(0.5f);
//    glm::vec3 outDir = glm::normalize(center);
//    glm::vec3 a = glm::abs(outDir);
//
//    if (a.x >= a.y && a.x >= a.z) return (outDir.x >= 0.0f) ? 0 : 1; // +X / -X
//    if (a.y >= a.x && a.y >= a.z) return (outDir.y >= 0.0f) ? 2 : 3; // +Y / -Y
//    return (outDir.z >= 0.0f) ? 4 : 5;                               // +Z / -Z
//}
//
//static inline glm::ivec2 TileForFaceOnVoxel(int faceIndex, const glm::ivec3& voxelWorld) {
//    int topFi = DominantTopFaceIndex(voxelWorld);
//    int bottomFi = topFi ^ 1;
//
//    if (faceIndex == topFi)         return TILE_TOP;
//    if (faceIndex == bottomFi)      return TILE_BOTTOM;
//    return SIDE_TILE[faceIndex];
//}
//
//
//// Key packing so we only merge faces that are truly compatible.
//static inline uint32_t MakeGreedyKey(Block b, int faceIndex, const glm::ivec2& tile) {
//    // bits:
//    //  0..7  : block (uint8)
//    //  8..10 : faceIndex (0..5)
//    // 11..12 : tile.x (0..3)
//    // 13..14 : tile.y (0..2)
//    return (uint32_t)b |
//        ((uint32_t)(faceIndex & 0x7) << 8) |
//        ((uint32_t)(tile.x & 0x3) << 11) |
//        ((uint32_t)(tile.y & 0x3) << 13);
//}
//static inline Block KeyBlock(uint32_t key) { return (Block)(key & 0xFFu); }
//static inline int   KeyFace(uint32_t key) { return (int)((key >> 8) & 0x7u); }
//static inline glm::ivec2 KeyTile(uint32_t key) {
//    return glm::ivec2((int)((key >> 11) & 0x3u), (int)((key >> 13) & 0x3u));
//}
//
//////////////////////////////////////////////////////
//// //////////////////  LONG FUNCTION ///////////////
//////////////////////////////////////////////////////
//template<typename IsSolidFn>
//static void BuildGreedyMeshPass(
//    const World& world,
//    const glm::ivec3& chunkBase,
//    int cubeNetW,
//    int cubeNetH,
//    IsSolidFn isSolid,
//    std::vector<VoxelVertex>& outVerts)
//{
//    outVerts.clear();
//    outVerts.reserve(8192);
//
//    struct Cell { bool empty = true; uint32_t key = 0; };
//    std::vector<Cell> mask(CHUNK_SIZE * CHUNK_SIZE);
//
//    // axis: 0=X, 1=Y, 2=Z
//    for (int axis = 0; axis < 3; axis++) {
//        int u = (axis + 1) % 3;
//        int v = (axis + 2) % 3;
//
//        // slice runs 0..CHUNK_SIZE (inclusive) so we can compare s-1 and s.
//        for (int s = 0; s <= CHUNK_SIZE; s++) {
//
//            // 1) Build the 2D mask for this slice.
//            for (int j = 0; j < CHUNK_SIZE; j++) {
//                for (int i = 0; i < CHUNK_SIZE; i++) {
//                    glm::ivec3 a(0), b(0);
//                    a[axis] = s - 1; // voxel on the "negative" side
//                    b[axis] = s;     // voxel on the "positive" side
//                    a[u] = i; a[v] = j;
//                    b[u] = i; b[v] = j;
//
//                    glm::ivec3 wa = chunkBase + a;
//                    glm::ivec3 wb = chunkBase + b;
//
//                    Block A = world.GetBlock(wa.x, wa.y, wa.z);
//                    Block B = world.GetBlock(wb.x, wb.y, wb.z);
//
//                    bool aSolid = isSolid(A);
//                    bool bSolid = isSolid(B);
//
//                    Cell cell{};
//
//                    // face exists if solidity differs in THIS pass.
//                    if (aSolid != bSolid) {
//                        bool solidIsA = aSolid;
//                        glm::ivec3 solidLocal = solidIsA ? a : b;
//
//                        // IMPORTANT: only emit faces for voxels that belong to THIS chunk.
//                        if (solidLocal[axis] >= 0 && solidLocal[axis] < CHUNK_SIZE) {
//                            Block faceBlock = solidIsA ? A : B;
//                            int sign = solidIsA ? +1 : -1;
//                            int faceIndex = axis * 2 + (sign < 0 ? 1 : 0);
//
//                            glm::ivec3 solidWorld = chunkBase + solidLocal;
//                            glm::ivec2 tile = TileForFaceOnVoxel(faceIndex, solidWorld);
//
//                            cell.empty = false;
//                            cell.key = MakeGreedyKey(faceBlock, faceIndex, tile);
//                        }
//                    }
//
//                    mask[i + CHUNK_SIZE * j] = cell;
//                }
//            }
//
//            // 2) Greedy merge rectangles in the mask.
//            for (int j = 0; j < CHUNK_SIZE; j++) {
//                for (int i = 0; i < CHUNK_SIZE; ) {
//
//                    Cell c = mask[i + CHUNK_SIZE * j];
//                    if (c.empty) { i++; continue; }
//
//                    uint32_t key = c.key;
//
//                    // Expand width
//                    int w = 1;
//                    while (i + w < CHUNK_SIZE) {
//                        Cell c2 = mask[(i + w) + CHUNK_SIZE * j];
//                        if (c2.empty || c2.key != key) break;
//                        w++;
//                    }
//
//                    // Expand height
//                    int h = 1;
//                    bool done = false;
//                    while (j + h < CHUNK_SIZE && !done) {
//                        for (int k = 0; k < w; k++) {
//                            Cell c3 = mask[(i + k) + CHUNK_SIZE * (j + h)];
//                            if (c3.empty || c3.key != key) { done = true; break; }
//                        }
//                        if (!done) h++;
//                    }
//
//                    // 3) Emit one quad for (i,j,w,h) on slice s.
//                    Block faceBlock = KeyBlock(key);
//                    int faceIndex = KeyFace(key);
//                    glm::ivec2 tile = KeyTile(key);
//
//                    glm::ivec3 p0(0), p1(0), p2(0), p3(0);
//                    p0[axis] = s; p1[axis] = s; p2[axis] = s; p3[axis] = s;
//                    p0[u] = i;     p0[v] = j;
//                    p1[u] = i + w; p1[v] = j;
//                    p2[u] = i + w; p2[v] = j + h;
//                    p3[u] = i;     p3[v] = j + h;
//
//                    glm::vec3 Q[4] = {
//                        glm::vec3(chunkBase + p0),
//                        glm::vec3(chunkBase + p1),
//                        glm::vec3(chunkBase + p2),
//                        glm::vec3(chunkBase + p3),
//                    };
//
//                    // Reorder corners to match FACES[faceIndex].c order.
//                    glm::vec3 P[4];
//                    for (int ci = 0; ci < 4; ci++) {
//                        P[ci] = Q[FACE_Q_MAP[faceIndex][ci]];
//                    }
//
//                    // NOTE: This will "stretch" the tile across the large quad.
//                    glm::vec2 UV[4];
//                    GetCubeNetUVsTile(faceIndex, tile.x, tile.y, cubeNetW, cubeNetH, UV);
//
//                    glm::vec3 normal = FACES[faceIndex].normal;
//                    float layer = (float)BlockLayer(faceBlock);
//
//                    auto push = [&](int ci) {
//                        outVerts.push_back({ P[ci], UV[ci], normal, layer });
//                        };
//
//                    push(0); push(1); push(2);
//                    push(0); push(2); push(3);
//
//                    // 4) Clear the used cells
//                    for (int y = 0; y < h; y++)
//                        for (int x = 0; x < w; x++)
//                            mask[(i + x) + CHUNK_SIZE * (j + y)].empty = true;
//
//                    i += w;
//                }
//            }
//        }
//    }
//}
////////////// END OF BuildGreedyMeshPass() //////////////////////
//////////////////////////////////////////////////////
//
//
//
//
//
//Chunk& World::GetOrCreateChunk(ChunkCoord cc) {
//    auto it = chunks.find(cc);
//    if (it != chunks.end()) return it->second;
//
//    Chunk c;
//    c.coord = cc;
//    auto [insIt, ok] = chunks.emplace(cc, std::move(c));
//    return insIt->second;
//}
//
//Block World::GetBlock(int wx, int wy, int wz) const {
//    ChunkCoord cc{
//        FloorDiv(wx, CHUNK_SIZE),
//        FloorDiv(wy, CHUNK_SIZE),
//        FloorDiv(wz, CHUNK_SIZE)
//    };
//
//    int lx = Mod(wx, CHUNK_SIZE);
//    int ly = Mod(wy, CHUNK_SIZE);
//    int lz = Mod(wz, CHUNK_SIZE);
//
//    auto it = chunks.find(cc);
//    if (it != chunks.end() && it->second.generated) {
//        return it->second.blocks[Idx(lx, ly, lz)];
//    }
//
//    // if missing OR not generated yet -> procedural fallback
//    glm::vec3 p = glm::vec3(wx, wy, wz) + glm::vec3(0.5f);
//    return SamplePlanetWithOcean(p, planet);
//
//}
//
//static void UploadMesh(GLuint& vao, GLuint& vbo,
//    const std::vector<VoxelVertex>& verts,
//    int& outCount)
//{
//    if (verts.empty()) { outCount = 0; return; }
//
//    if (vao == 0) glGenVertexArrays(1, &vao);
//    if (vbo == 0) glGenBuffers(1, &vbo);
//
//    glBindVertexArray(vao);
//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glBufferData(GL_ARRAY_BUFFER,
//        verts.size() * sizeof(VoxelVertex),
//        verts.data(),
//        GL_STATIC_DRAW);
//
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, pos));
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, uv));
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, normal));
//    glEnableVertexAttribArray(3);
//    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, layer));
//
//    glBindVertexArray(0);
//
//    outCount = (int)verts.size();
//}
//
//void World::FillChunkBlocks(Chunk& c) {
//    for (int z = 0; z < CHUNK_SIZE; z++)
//        for (int y = 0; y < CHUNK_SIZE; y++)
//            for (int x = 0; x < CHUNK_SIZE; x++) {
//                int wx = c.coord.x * CHUNK_SIZE + x;
//                int wy = c.coord.y * CHUNK_SIZE + y;
//                int wz = c.coord.z * CHUNK_SIZE + z;
//
//                glm::vec3 p = glm::vec3(wx, wy, wz) + glm::vec3(0.5f);
//                c.blocks[Idx(x, y, z)] = SamplePlanetWithOcean(p, planet);
//            }
//    c.dirty = true;
//    c.generated = true;
//}
//
//void World::DrawOpaque() const {
//    for (auto& [cc, c] : chunks) {
//        if (c.opaqueCount == 0) continue;
//        glBindVertexArray(c.vaoOpaque);
//        glDrawArrays(GL_TRIANGLES, 0, c.opaqueCount);
//    }
//    glBindVertexArray(0);
//}
//
//void World::DrawWater() const {
//    for (auto& [cc, c] : chunks) {
//        if (c.waterCount == 0) continue;
//        glBindVertexArray(c.vaoWater);
//        glDrawArrays(GL_TRIANGLES, 0, c.waterCount);
//    }
//    glBindVertexArray(0);
//}
//
//
//void World::BuildChunkMesh(Chunk& c) {
//    glm::ivec3 chunkBase(
//        c.coord.x * CHUNK_SIZE,
//        c.coord.y * CHUNK_SIZE,
//        c.coord.z * CHUNK_SIZE
//    );
//
//    std::vector<VoxelVertex> opaqueVerts;
//    std::vector<VoxelVertex> waterVerts;
//
//    // Pass 1: Opaque blocks (treat Water like "air" so solids render against it)
//    BuildGreedyMeshPass(*this, chunkBase, cubeNetW, cubeNetH,
//        [](Block b) { return IsOpaque(b); },
//        opaqueVerts);
//
//    // Pass 2: Water blocks (only Water is solid for this pass)
//    BuildGreedyMeshPass(*this, chunkBase, cubeNetW, cubeNetH,
//        [](Block b) { return b == Block::Water; },
//        waterVerts);
//
//    UploadMesh(c.vaoOpaque, c.vboOpaque, opaqueVerts, c.opaqueCount);
//    UploadMesh(c.vaoWater, c.vboWater, waterVerts, c.waterCount);
//
//    c.dirty = false;
//}
//
//void World::BuildPlanetOnce() {
//    // Build a whole small planet in a bounded chunk cube.
//    float rMax = planet.baseRadius + planet.maxHeight + 2.0f;
//    int minW = (int)std::floor(-rMax);
//    int maxW = (int)std::ceil(rMax);
//
//    int minC = FloorDiv(minW, CHUNK_SIZE);
//    int maxC = FloorDiv(maxW, CHUNK_SIZE);
//
//    // 1) create + fill chunks
//    for (int cz = minC; cz <= maxC; cz++)
//        for (int cy = minC; cy <= maxC; cy++)
//            for (int cx = minC; cx <= maxC; cx++) {
//                Chunk& c = GetOrCreateChunk({ cx,cy,cz });
//                FillChunkBlocks(c);
//            }
//
//    // 2) build meshes
//    RebuildDirtyMeshes();
//}
//
//void World::RebuildDirtyMeshes() {
//    for (auto& [cc, c] : chunks) {
//        if (c.dirty) BuildChunkMesh(c);
//    }
//}
//
////void World::Draw() const {
////    for (auto& [cc, c] : chunks) {
////        if (c.vertexCount == 0) continue;
////        glBindVertexArray(c.vao);
////        glDrawArrays(GL_TRIANGLES, 0, c.vertexCount);
////    }
////    glBindVertexArray(0);
////}
//
//glm::ivec3 CameraVoxel(glm::vec3 camPos) {
//    return glm::ivec3((int)std::floor(camPos.x),
//        (int)std::floor(camPos.y),
//        (int)std::floor(camPos.z));
//}
//
//extern ChunkCoord CameraChunk(glm::vec3 camPos) {
//    glm::ivec3 v = CameraVoxel(camPos);
//   // std::cout << v.x << " " << v.y << " " << v.z << std::endl;
//    return WorldToChunk(v.x, v.y, v.z);
//}
//
//struct Candidate { ChunkCoord cc; int dist2; };
//
//
//
//void World::UpdateStreaming(glm::vec3 cameraPos)
//{
//    ChunkCoord cc = CameraChunk(cameraPos);
//    std::vector<Candidate> cand;
//
//    for (int dz = -renderDistance; dz <= renderDistance; dz++)
//        for (int dy = -renderDistance; dy <= renderDistance; dy++)
//            for (int dx = -renderDistance; dx <= renderDistance; dx++)
//            {
//                int dist2 = dx * dx + dy * dy + dz * dz;
//                ChunkCoord want{ cc.x + dx, cc.y + dy, cc.z + dz };
//                cand.push_back({ want,dist2 });
//
//               
//            }
//
//    std::sort(cand.begin(), cand.end(), [](auto& a, auto& b) { return a.dist2 < b.dist2; });
//    for (auto& want : cand)
//    {
//        if (chunks.find(want.cc) == chunks.end())
//        {
//            // insert the empty chunk now so we dont enqueue duplicates
//            Chunk c; c.coord = want.cc;
//            chunks.emplace(want.cc, std::move(c));
//            genQueue.push_back(want.cc);
//        }
//    }
//
//    // Unloa passs (done after, so we dont delete while iterating)
//    std::vector<ChunkCoord> toDelete;
//    for (auto& [coord, chunk] : chunks)
//    {
//        int ddx = coord.x - cc.x;
//        int ddy = coord.y - cc.y;
//        int ddz = coord.z - cc.z;
//        int distCheby = std::max<int>({ std::abs(ddx), std::abs(ddy), std::abs(ddz) });
//
//        if (distCheby > unloadDistance) 
//        {
//            toDelete.push_back(coord);
//
//        }
//
//    }
//
//    for (auto& coord : toDelete)
//    {
//        //Free GPU buffers too
//        auto it = chunks.find(coord);
//        if (it != chunks.end())
//        {
//            Chunk& c = it->second;
//            if (c.vaoOpaque) glDeleteVertexArrays(1, &c.vaoOpaque);
//            if (c.vaoWater) glDeleteVertexArrays(1, &c.vaoWater);
//
//            if (c.vboOpaque) glDeleteBuffers(1, &c.vboOpaque);
//            if (c.vboWater) glDeleteBuffers(1, &c.vboWater);
//
//            chunks.erase(it);
//        }
//    }
//}
//
//
//bool ChunkAllAir(const Chunk& c)
//{
//    for (Block b : c.blocks) if (b != Block::Air) return false;
//    return true;
//}
//
//void World::TickBuildQueues(int maxGenPerFrame, int maxMeshPerFrame)
//{
//    // 1) Generate blocks
//    for (int i = 0; i < maxGenPerFrame && !genQueue.empty(); i++)
//    {
//        ChunkCoord cc = genQueue.front();
//        genQueue.pop_front();
//
//        auto it = chunks.find(cc);
//        if (it == chunks.end()) continue;
//
//        Chunk& c = it->second;
//
//        static const ChunkCoord N6[6] = {
//    { 1,0,0}, {-1,0,0},
//    { 0,1,0}, { 0,-1,0},
//    { 0,0,1}, { 0,0,-1}
//        };
//
//
//        FillChunkBlocks(c);
//        meshQueue.push_back(cc);
//
//        // neighbors
//        for (auto d : N6) {
//            ChunkCoord n{ cc.x + d.x, cc.y + d.y, cc.z + d.z };
//            auto itN = chunks.find(n);
//            if (itN != chunks.end() && itN->second.generated) {
//                meshQueue.push_back(n);
//            }
//        }
//    }
//
//    bool doRender = false;
//    for (auto& cc : chunks)
//    { 
//        if (!ChunkAllAir(cc.second))
//        {
//            doRender = true;
//            break;
//        }
//    }
//    if (!doRender) return;
//
//    // 2) build meshes
//    for (int i = 0; i < maxMeshPerFrame && !meshQueue.empty(); i++)
//    {
//        ChunkCoord cc = meshQueue.front();
//        meshQueue.pop_front();
//
//        auto it = chunks.find(cc);
//        if (it == chunks.end()) continue;
//
//        Chunk& c = it->second;
//        BuildChunkMesh(c);
//
//        static double t0 = glfwGetTime();
//        double t = glfwGetTime();
//        if (t - t0 > 1.0) {
//            t0 = t;
//            std::cout << "Chunk " << cc.x << "," << cc.y << "," << cc.z
//                << " opaque=" << c.opaqueCount
//                << " water=" << c.waterCount << "\n";
//        }
//
//
//
//    }
//}

#include "World.h"
#include "Mesher.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <algorithm>
#include <iostream>

struct VoxelVertex { glm::vec3 pos; glm::vec2 uv; glm::vec3 normal; float layer; };
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


// --- Greedy meshing helpers --------------------------------------------------

// Map from "canonical quad corners" Q[0..3] (made from (i,j,w,h) in u/v order)
// into the corner order expected by FACES[faceIndex].c[0..3] + GetCubeNetUVsTile.
static const uint8_t FACE_Q_MAP[6][4] = {
    {0,1,2,3}, // +X
    {3,2,1,0}, // -X
    {1,2,3,0}, // +Y
    {0,3,2,1}, // -Y
    {1,2,3,0}, // +Z
    {0,3,2,1}, // -Z
};

static inline int DominantTopFaceIndex(const glm::ivec3& voxelWorld) {
    // Same logic as the naive mesher: pick the dominant axis of the voxel center's
    // normalized vector from the planet center (0,0,0).
    glm::vec3 center = glm::vec3(voxelWorld) + glm::vec3(0.5f);
    glm::vec3 outDir = glm::normalize(center);
    glm::vec3 a = glm::abs(outDir);

    if (a.x >= a.y && a.x >= a.z) return (outDir.x >= 0.0f) ? 0 : 1; // +X / -X
    if (a.y >= a.x && a.y >= a.z) return (outDir.y >= 0.0f) ? 2 : 3; // +Y / -Y
    return (outDir.z >= 0.0f) ? 4 : 5;                               // +Z / -Z
}

static inline glm::ivec2 TileForFaceOnVoxel(int faceIndex, const glm::ivec3& voxelWorld) {
    int topFi = DominantTopFaceIndex(voxelWorld);
    int bottomFi = topFi ^ 1;

    if (faceIndex == topFi)         return TILE_TOP;
    if (faceIndex == bottomFi)      return TILE_BOTTOM;
    return SIDE_TILE[faceIndex];
}

// Key packing so we only merge faces that are truly compatible.
static inline uint32_t MakeGreedyKey(Block b, int faceIndex, const glm::ivec2& tile) {
    // bits:
    //  0..7  : block (uint8)
    //  8..10 : faceIndex (0..5)
    // 11..12 : tile.x (0..3)
    // 13..14 : tile.y (0..2)
    return (uint32_t)b |
        ((uint32_t)(faceIndex & 0x7) << 8) |
        ((uint32_t)(tile.x & 0x3) << 11) |
        ((uint32_t)(tile.y & 0x3) << 13);
}
static inline Block KeyBlock(uint32_t key) { return (Block)(key & 0xFFu); }
static inline int   KeyFace(uint32_t key) { return (int)((key >> 8) & 0x7u); }
static inline glm::ivec2 KeyTile(uint32_t key) {
    return glm::ivec2((int)((key >> 11) & 0x3u), (int)((key >> 13) & 0x3u));
}

template<typename IsSolidFn>
static void BuildGreedyMeshPass(
    const World& world,
    const glm::ivec3& chunkBase,
    int cubeNetW,
    int cubeNetH,
    IsSolidFn isSolid,
    std::vector<VoxelVertex>& outVerts)
{
    outVerts.clear();
    outVerts.reserve(8192);

    struct Cell { bool empty = true; uint32_t key = 0; };
    std::vector<Cell> mask(CHUNK_SIZE * CHUNK_SIZE);

    // axis: 0=X, 1=Y, 2=Z
    for (int axis = 0; axis < 3; axis++) {
        int u = (axis + 1) % 3;
        int v = (axis + 2) % 3;

        // slice runs 0..CHUNK_SIZE (inclusive) so we can compare s-1 and s.
        for (int s = 0; s <= CHUNK_SIZE; s++) {

            // 1) Build the 2D mask for this slice.
            for (int j = 0; j < CHUNK_SIZE; j++) {
                for (int i = 0; i < CHUNK_SIZE; i++) {
                    glm::ivec3 a(0), b(0);
                    a[axis] = s - 1; // voxel on the "negative" side
                    b[axis] = s;     // voxel on the "positive" side
                    a[u] = i; a[v] = j;
                    b[u] = i; b[v] = j;

                    glm::ivec3 wa = chunkBase + a;
                    glm::ivec3 wb = chunkBase + b;

                    Block A = world.GetBlock(wa.x, wa.y, wa.z);
                    Block B = world.GetBlock(wb.x, wb.y, wb.z);

                    bool aSolid = isSolid(A);
                    bool bSolid = isSolid(B);

                    Cell cell{};

                    // face exists if solidity differs in THIS pass.
                    if (aSolid != bSolid) {
                        bool solidIsA = aSolid;
                        glm::ivec3 solidLocal = solidIsA ? a : b;

                        // IMPORTANT: only emit faces for voxels that belong to THIS chunk.
                        // This matches your naive mesher (which only iterates local voxels).
                        if (solidLocal[axis] >= 0 && solidLocal[axis] < CHUNK_SIZE) {
                            Block faceBlock = solidIsA ? A : B;
                            int sign = solidIsA ? +1 : -1;
                            int faceIndex = axis * 2 + (sign < 0 ? 1 : 0);

                            glm::ivec3 solidWorld = chunkBase + solidLocal;
                            glm::ivec2 tile = TileForFaceOnVoxel(faceIndex, solidWorld);

                            cell.empty = false;
                            cell.key = MakeGreedyKey(faceBlock, faceIndex, tile);
                        }
                    }

                    mask[i + CHUNK_SIZE * j] = cell;
                }
            }

            // 2) Greedy merge rectangles in the mask.
            for (int j = 0; j < CHUNK_SIZE; j++) {
                for (int i = 0; i < CHUNK_SIZE; ) {

                    Cell c = mask[i + CHUNK_SIZE * j];
                    if (c.empty) { i++; continue; }

                    uint32_t key = c.key;

                    // Expand width
                    int w = 1;
                    while (i + w < CHUNK_SIZE) {
                        Cell c2 = mask[(i + w) + CHUNK_SIZE * j];
                        if (c2.empty || c2.key != key) break;
                        w++;
                    }

                    // Expand height (row by row must match full width)
                    int h = 1;
                    bool done = false;
                    while (j + h < CHUNK_SIZE && !done) {
                        for (int k = 0; k < w; k++) {
                            Cell c3 = mask[(i + k) + CHUNK_SIZE * (j + h)];
                            if (c3.empty || c3.key != key) { done = true; break; }
                        }
                        if (!done) h++;
                    }

                    // 3) Emit one quad for (i,j,w,h) on slice s.
                    Block faceBlock = KeyBlock(key);
                    int faceIndex = KeyFace(key);
                    glm::ivec2 tile = KeyTile(key);

                    glm::ivec3 p0(0), p1(0), p2(0), p3(0);
                    p0[axis] = s; p1[axis] = s; p2[axis] = s; p3[axis] = s;
                    p0[u] = i;     p0[v] = j;
                    p1[u] = i + w; p1[v] = j;
                    p2[u] = i + w; p2[v] = j + h;
                    p3[u] = i;     p3[v] = j + h;

                    glm::vec3 Q[4] = {
                        glm::vec3(chunkBase + p0),
                        glm::vec3(chunkBase + p1),
                        glm::vec3(chunkBase + p2),
                        glm::vec3(chunkBase + p3),
                    };

                    // Reorder corners to match FACES[faceIndex].c order.
                    glm::vec3 P[4];
                    for (int ci = 0; ci < 4; ci++) {
                        P[ci] = Q[FACE_Q_MAP[faceIndex][ci]];
                    }

                    // NOTE: This will "stretch" the tile across the large quad.
                    // (Next upgrade: true per-block tiling inside a cube-net tile.)
                    glm::vec2 UV[4];
                    GetCubeNetUVsTile(faceIndex, tile.x, tile.y, cubeNetW, cubeNetH, UV);

                    glm::vec3 normal = FACES[faceIndex].normal;
                    float layer = (float)BlockLayer(faceBlock);

                    auto push = [&](int ci) {
                        outVerts.push_back({ P[ci], UV[ci], normal, layer });
                        };

                    push(0); push(1); push(2);
                    push(0); push(2); push(3);

                    // 4) Clear the used cells
                    for (int y = 0; y < h; y++)
                        for (int x = 0; x < w; x++)
                            mask[(i + x) + CHUNK_SIZE * (j + y)].empty = true;

                    i += w;
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------


Chunk& World::GetOrCreateChunk(ChunkCoord cc) {
    auto it = chunks.find(cc);
    if (it != chunks.end()) return it->second;

    Chunk c;
    c.coord = cc;
    auto [insIt, ok] = chunks.emplace(cc, std::move(c));
    return insIt->second;
}

Block World::GetBlock(int wx, int wy, int wz) const {
    ChunkCoord cc{
        FloorDiv(wx, CHUNK_SIZE),
        FloorDiv(wy, CHUNK_SIZE),
        FloorDiv(wz, CHUNK_SIZE)
    };

    int lx = Mod(wx, CHUNK_SIZE);
    int ly = Mod(wy, CHUNK_SIZE);
    int lz = Mod(wz, CHUNK_SIZE);

    auto it = chunks.find(cc);
    if (it != chunks.end() && it->second.generated) {
        return it->second.blocks[Idx(lx, ly, lz)];
    }

    // if missing OR not generated yet -> procedural fallback
    glm::vec3 p = glm::vec3(wx, wy, wz) + glm::vec3(0.5f);
    return SamplePlanetWithOcean(p, planet);

}

static void UploadMesh(GLuint& vao, GLuint& vbo,
    const std::vector<VoxelVertex>& verts,
    int& outCount)
{
    if (verts.empty()) { outCount = 0; return; }

    if (vao == 0) glGenVertexArrays(1, &vao);
    if (vbo == 0) glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(VoxelVertex),
        verts.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, normal));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, layer));

    glBindVertexArray(0);

    outCount = (int)verts.size();
}

void World::FillChunkBlocks(Chunk& c) {
    for (int z = 0; z < CHUNK_SIZE; z++)
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                int wx = c.coord.x * CHUNK_SIZE + x;
                int wy = c.coord.y * CHUNK_SIZE + y;
                int wz = c.coord.z * CHUNK_SIZE + z;

                glm::vec3 p = glm::vec3(wx, wy, wz) + glm::vec3(0.5f);
                c.blocks[Idx(x, y, z)] = SamplePlanetWithOcean(p, planet);
            }
    c.dirty = true;
    c.generated = true;
}

void World::DrawOpaque() const {
    for (auto& [cc, c] : chunks) {
        if (c.opaqueCount == 0) continue;
        glBindVertexArray(c.vaoOpaque);
        glDrawArrays(GL_TRIANGLES, 0, c.opaqueCount);
    }
    glBindVertexArray(0);
}

void World::DrawWater() const {
    for (auto& [cc, c] : chunks) {
        if (c.waterCount == 0) continue;
        glBindVertexArray(c.vaoWater);
        glDrawArrays(GL_TRIANGLES, 0, c.waterCount);
    }
    glBindVertexArray(0);
}


void World::BuildChunkMesh(Chunk& c) {
    glm::ivec3 chunkBase(
        c.coord.x * CHUNK_SIZE,
        c.coord.y * CHUNK_SIZE,
        c.coord.z * CHUNK_SIZE
    );

    std::vector<VoxelVertex> opaqueVerts;
    std::vector<VoxelVertex> waterVerts;

    // Pass 1: Opaque blocks (treat Water like "air" so solids render against it)
    BuildGreedyMeshPass(*this, chunkBase, cubeNetW, cubeNetH,
        [](Block b) { return IsOpaque(b); },
        opaqueVerts);

    // Pass 2: Water blocks (only Water is solid for this pass)
    BuildGreedyMeshPass(*this, chunkBase, cubeNetW, cubeNetH,
        [](Block b) { return b == Block::Water; },
        waterVerts);

    UploadMesh(c.vaoOpaque, c.vboOpaque, opaqueVerts, c.opaqueCount);
    UploadMesh(c.vaoWater, c.vboWater, waterVerts, c.waterCount);

    c.dirty = false;
}


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

//void World::Draw() const {
//    for (auto& [cc, c] : chunks) {
//        if (c.vertexCount == 0) continue;
//        glBindVertexArray(c.vao);
//        glDrawArrays(GL_TRIANGLES, 0, c.vertexCount);
//    }
//    glBindVertexArray(0);
//}

glm::ivec3 CameraVoxel(glm::vec3 camPos) {
    return glm::ivec3((int)std::floor(camPos.x),
        (int)std::floor(camPos.y),
        (int)std::floor(camPos.z));
}

extern ChunkCoord CameraChunk(glm::vec3 camPos) {
    glm::ivec3 v = CameraVoxel(camPos);
    // std::cout << v.x << " " << v.y << " " << v.z << std::endl;
    return WorldToChunk(v.x, v.y, v.z);
}

struct Candidate { ChunkCoord cc; int dist2; };



void World::UpdateStreaming(glm::vec3 cameraPos)
{
    ChunkCoord cc = CameraChunk(cameraPos);
    std::vector<Candidate> cand;

    for (int dz = -renderDistance; dz <= renderDistance; dz++)
        for (int dy = -renderDistance; dy <= renderDistance; dy++)
            for (int dx = -renderDistance; dx <= renderDistance; dx++)
            {
                int dist2 = dx * dx + dy * dy + dz * dz;
                ChunkCoord want{ cc.x + dx, cc.y + dy, cc.z + dz };
                cand.push_back({ want,dist2 });


            }

    std::sort(cand.begin(), cand.end(), [](auto& a, auto& b) { return a.dist2 < b.dist2; });
    for (auto& want : cand)
    {
        if (chunks.find(want.cc) == chunks.end())
        {
            // insert the empty chunk now so we dont enqueue duplicates
            Chunk c; c.coord = want.cc;
            chunks.emplace(want.cc, std::move(c));
            genQueue.push_back(want.cc);
        }
    }

    // Unloa passs (done after, so we dont delete while iterating)
    std::vector<ChunkCoord> toDelete;
    for (auto& [coord, chunk] : chunks)
    {
        int ddx = coord.x - cc.x;
        int ddy = coord.y - cc.y;
        int ddz = coord.z - cc.z;
        int distCheby = std::max<int>({ std::abs(ddx), std::abs(ddy), std::abs(ddz) });

        if (distCheby > unloadDistance)
        {
            toDelete.push_back(coord);

        }

    }

    for (auto& coord : toDelete)
    {
        //Free GPU buffers too
        auto it = chunks.find(coord);
        if (it != chunks.end())
        {
            Chunk& c = it->second;
            if (c.vaoOpaque) glDeleteVertexArrays(1, &c.vaoOpaque);
            if (c.vaoWater) glDeleteVertexArrays(1, &c.vaoWater);

            if (c.vboOpaque) glDeleteBuffers(1, &c.vboOpaque);
            if (c.vboWater) glDeleteBuffers(1, &c.vboWater);

            chunks.erase(it);
        }
    }
}


bool ChunkAllAir(const Chunk& c)
{
    for (Block b : c.blocks) if (b != Block::Air) return false;
    return true;
}

void World::TickBuildQueues(int maxGenPerFrame, int maxMeshPerFrame)
{
    // 1) Generate blocks
    for (int i = 0; i < maxGenPerFrame && !genQueue.empty(); i++)
    {
        ChunkCoord cc = genQueue.front();
        genQueue.pop_front();

        auto it = chunks.find(cc);
        if (it == chunks.end()) continue;

        Chunk& c = it->second;

        static const ChunkCoord N6[6] = {
    { 1,0,0}, {-1,0,0},
    { 0,1,0}, { 0,-1,0},
    { 0,0,1}, { 0,0,-1}
        };


        FillChunkBlocks(c);
        meshQueue.push_back(cc);

        // neighbors
        for (auto d : N6) {
            ChunkCoord n{ cc.x + d.x, cc.y + d.y, cc.z + d.z };
            auto itN = chunks.find(n);
            if (itN != chunks.end() && itN->second.generated) {
                meshQueue.push_back(n);
            }
        }
    }

    bool doRender = false;
    for (auto& cc : chunks)
    {
        if (!ChunkAllAir(cc.second))
        {
            doRender = true;
            break;
        }
    }
    if (!doRender) return;

    // 2) build meshes
    for (int i = 0; i < maxMeshPerFrame && !meshQueue.empty(); i++)
    {
        ChunkCoord cc = meshQueue.front();
        meshQueue.pop_front();

        auto it = chunks.find(cc);
        if (it == chunks.end()) continue;

        Chunk& c = it->second;
        BuildChunkMesh(c);

        static double t0 = glfwGetTime();
        double t = glfwGetTime();
        if (t - t0 > 1.0) {
            t0 = t;
            std::cout << "Chunk " << cc.x << "," << cc.y << "," << cc.z
                << " opaque=" << c.opaqueCount
                << " water=" << c.waterCount << "\n";
        }



    }
}


