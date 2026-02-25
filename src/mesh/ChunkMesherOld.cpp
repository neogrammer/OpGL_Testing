#include "ChunkMesher.h"
#include "src/voxel/Mesher.h"

#include <algorithm>
#include <cstdint>

// --- Tile selection (same logic you had in World.cpp) ------------------------

static const glm::ivec2 TILE_TOP = { 1,2 };
static const glm::ivec2 TILE_BOTTOM = { 1,0 };

static const glm::ivec2 SIDE_TILE[6] = {
    {2,1}, // +X
    {0,1}, // -X
    {1,1}, // +Y as side
    {1,1}, // -Y as side
    {1,1}, // +Z
    {3,1}, // -Z
};

static inline int DominantTopFaceIndex(const glm::ivec3& voxelWorld)
{
    glm::vec3 center = glm::vec3(voxelWorld) + glm::vec3(0.5f);
    glm::vec3 outDir = glm::normalize(center);
    glm::vec3 a = glm::abs(outDir);

    if (a.x >= a.y && a.x >= a.z) return (outDir.x >= 0.0f) ? 0 : 1; // +X/-X
    if (a.y >= a.x && a.y >= a.z) return (outDir.y >= 0.0f) ? 2 : 3; // +Y/-Y
    return (outDir.z >= 0.0f) ? 4 : 5;                               // +Z/-Z
}

static inline glm::ivec2 TileForFaceOnVoxel(int faceIndex, const glm::ivec3& voxelWorld)
{
    int topFi = DominantTopFaceIndex(voxelWorld);
    int bottomFi = topFi ^ 1;

    if (faceIndex == topFi)    return TILE_TOP;
    if (faceIndex == bottomFi) return TILE_BOTTOM;
    return SIDE_TILE[faceIndex];
}

// Use local blocks when inside the chunk; otherwise call getBlockWorld.
static inline Block GetBlockLocalOrWorld(
    const std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>& blocks,
    const glm::ivec3& chunkBase,
    const GetBlockFn& getBlockWorld,
    int lx, int ly, int lz)
{
    if (lx >= 0 && lx < CHUNK_SIZE &&
        ly >= 0 && ly < CHUNK_SIZE &&
        lz >= 0 && lz < CHUNK_SIZE)
    {
        return blocks[Idx(lx, ly, lz)];
    }

    glm::ivec3 w = chunkBase + glm::ivec3(lx, ly, lz);
    return getBlockWorld(w.x, w.y, w.z);
}

// ----------------------------------------------------------------------------
// 1) FACE-CULLED MESHER (this is your old BuildChunkMesh moved out cleanly)
// ----------------------------------------------------------------------------
ChunkMeshData BuildChunkMeshFaceCulled(
    const std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>& blocks,
    const glm::ivec3& chunkBase,
    const GetBlockFn& getBlockWorld,
    int cubeNetW, int cubeNetH)
{
    ChunkMeshData out;
    out.opaque.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6);
    out.water.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6);

    for (int z = 0; z < CHUNK_SIZE; z++)
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++)
            {
                Block b = blocks[Idx(x, y, z)];
                if (!IsSolid(b)) continue;

                glm::ivec3 voxelWorld = chunkBase + glm::ivec3(x, y, z);
                glm::vec3 base = glm::vec3(voxelWorld);

                int topFi = DominantTopFaceIndex(voxelWorld);
                int bottomFi = topFi ^ 1;

                for (int fi = 0; fi < 6; fi++)
                {
                    const FaceDef& f = FACES[fi];

                    Block nb = GetBlockLocalOrWorld(blocks, chunkBase, getBlockWorld,
                        x + f.dir.x, y + f.dir.y, z + f.dir.z);

                    if (!ShouldRenderFace(b, nb)) continue;

                    glm::ivec2 tile;
                    if (fi == topFi)        tile = TILE_TOP;
                    else if (fi == bottomFi) tile = TILE_BOTTOM;
                    else                    tile = SIDE_TILE[fi];

                    glm::vec2 faceUV[4];
                    GetCubeNetUVsTile(fi, tile.x, tile.y, cubeNetW, cubeNetH, faceUV);

                    auto& verts = (b == Block::Water) ? out.water : out.opaque;

                    auto push = [&](int ci) {
                        VoxelVertex v{}; // IMPORTANT: zero-init so new fields aren't garbage
                        v.pos = base + glm::vec3(f.c[ci]);
                        v.localUV = UV4[ci];                              // 0..1 local face UV
                        v.normal = f.normal;
                        v.layer = (float)BlockLayer(b);
                        v.tile = glm::vec2((float)tile.x, (float)tile.y); // <-- REQUIRED
                        verts.push_back(v);
                        };

                    push(0); push(1); push(2);
                    push(0); push(2); push(3);
                }
            }

    return out;
}

// ----------------------------------------------------------------------------
// 2) GREEDY MESHER (full implementation, ready when you flip the switch)
// ----------------------------------------------------------------------------

static const uint8_t FACE_Q_MAP[6][4] = {
    {0,1,2,3}, // +X
    {3,2,1,0}, // -X
    {1,2,3,0}, // +Y
    {0,3,2,1}, // -Y
    {1,2,3,0}, // +Z
    {0,3,2,1}, // -Z
};

static inline uint32_t MakeGreedyKey(Block b, int faceIndex, const glm::ivec2& tile)
{
    // 0..7   block
    // 8..10  faceIndex (0..5)
    // 11..12 tile.x (0..3)
    // 13..14 tile.y (0..2)
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
static void BuildGreedyPass(
    const std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>& blocks,
    const glm::ivec3& chunkBase,
    const GetBlockFn& getBlockWorld,
    int cubeNetW, int cubeNetH,
    IsSolidFn isSolid,
    std::vector<VoxelVertex>& outVerts)
{
    outVerts.clear();
    outVerts.reserve(8192);

    struct Cell { bool empty = true; uint32_t key = 0; };
    std::vector<Cell> mask(CHUNK_SIZE * CHUNK_SIZE);

    for (int axis = 0; axis < 3; axis++)
    {
        int u = (axis + 1) % 3;
        int v = (axis + 2) % 3;

        for (int s = 0; s <= CHUNK_SIZE; s++)
        {
            // Build mask
            for (int j = 0; j < CHUNK_SIZE; j++)
                for (int i = 0; i < CHUNK_SIZE; i++)
                {
                    glm::ivec3 a(0), b(0);
                    a[axis] = s - 1;
                    b[axis] = s;
                    a[u] = i; a[v] = j;
                    b[u] = i; b[v] = j;

                    Block A = GetBlockLocalOrWorld(blocks, chunkBase, getBlockWorld, a.x, a.y, a.z);
                    Block B = GetBlockLocalOrWorld(blocks, chunkBase, getBlockWorld, b.x, b.y, b.z);

                    bool aSolid = isSolid(A);
                    bool bSolid = isSolid(B);

                    Cell cell{};

                    if (aSolid != bSolid)
                    {
                        bool solidIsA = aSolid;
                        glm::ivec3 solidLocal = solidIsA ? a : b;

                        // Only the chunk that OWNS the solid voxel emits the face.
                        if (solidLocal.x >= 0 && solidLocal.x < CHUNK_SIZE &&
                            solidLocal.y >= 0 && solidLocal.y < CHUNK_SIZE &&
                            solidLocal.z >= 0 && solidLocal.z < CHUNK_SIZE)
                        {
                            Block faceBlock = solidIsA ? A : B;
                            Block otherBlock = solidIsA ? B : A;
                            int faceIndex = axis * 2 + (solidIsA ? 0 : 1);

                            // Skip water faces against anything except AIR (prevents z-fighting with terrain)
                            if (faceBlock == Block::Water && otherBlock != Block::Air) {
                                // leave cell empty
                            }
                            else {
                                glm::ivec3 solidWorld = chunkBase + solidLocal;
                                glm::ivec2 tile = TileForFaceOnVoxel(faceIndex, solidWorld);

                                cell.empty = false;
                                cell.key = MakeGreedyKey(faceBlock, faceIndex, tile);
                            }
                        }
                    }

                    mask[i + CHUNK_SIZE * j] = cell;
                }

            // Merge rectangles + emit
            for (int j = 0; j < CHUNK_SIZE; j++)
            {
                for (int i = 0; i < CHUNK_SIZE; )
                {
                    Cell c = mask[i + CHUNK_SIZE * j];
                    if (c.empty) { i++; continue; }

                    uint32_t key = c.key;

                    int w = 1;
                    while (i + w < CHUNK_SIZE) {
                        Cell c2 = mask[(i + w) + CHUNK_SIZE * j];
                        if (c2.empty || c2.key != key) break;
                        w++;
                    }

                    int h = 1;
                    bool stop = false;
                    while (j + h < CHUNK_SIZE && !stop) {
                        for (int k = 0; k < w; k++) {
                            Cell c3 = mask[(i + k) + CHUNK_SIZE * (j + h)];
                            if (c3.empty || c3.key != key) { stop = true; break; }
                        }
                        if (!stop) h++;
                    }

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

                    glm::vec3 P[4];
                    for (int ci = 0; ci < 4; ci++) {
                        P[ci] = Q[FACE_Q_MAP[faceIndex][ci]];
                    }

                    // UV4 convention:
// edge 0->1 is V direction, edge 1->2 is U direction
float vSpan = glm::length(P[1] - P[0]); // becomes how many blocks tall
float uSpan = glm::length(P[2] - P[1]); // becomes how many blocks wide

                    glm::vec2 faceUV[4];
                    GetCubeNetUVsTile(faceIndex, tile.x, tile.y, cubeNetW, cubeNetH, faceUV);

                    glm::vec3 normal = FACES[faceIndex].normal;
                    float layer = (float)BlockLayer(faceBlock);
                    auto push = [&](int ci) {
                        VoxelVertex v{};
                        v.pos = P[ci];

                        // IMPORTANT: make UVs tile per-voxel across greedy quads
                        v.localUV = glm::vec2(UV4[ci].x * uSpan, UV4[ci].y * vSpan);

                        v.normal = normal;
                        v.layer = layer;
                        v.tile = glm::vec2((float)tile.x, (float)tile.y);
                        outVerts.push_back(v);
                        };
                    //auto push = [&](int ci) {
                    //    outVerts.push_back({ P[ci], faceUV[ci], normal, layer });
                    //    };

                    push(0); push(1); push(2);
                    push(0); push(2); push(3);

                    for (int yy = 0; yy < h; yy++)
                        for (int xx = 0; xx < w; xx++)
                            mask[(i + xx) + CHUNK_SIZE * (j + yy)].empty = true;

                    i += w;
                }
            }
        }
    }
}

ChunkMeshData BuildChunkMeshGreedy(
    const std::array<Block, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>& blocks,
    const glm::ivec3& chunkBase,
    const GetBlockFn& getBlockWorld,
    int cubeNetW, int cubeNetH)
{
    ChunkMeshData out;

    // Opaque pass: treat water as "air"
    BuildGreedyPass(blocks, chunkBase, getBlockWorld, cubeNetW, cubeNetH,
        [](Block b) { return IsOpaque(b); },
        out.opaque);

    // Water pass: only water is solid
    BuildGreedyPass(blocks, chunkBase, getBlockWorld, cubeNetW, cubeNetH,
        [](Block b) { return b == Block::Water; },
        out.water);

    return out;
}