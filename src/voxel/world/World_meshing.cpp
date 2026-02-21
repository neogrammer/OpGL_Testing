#include "../World.h"
#include "World_render.cpp"




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