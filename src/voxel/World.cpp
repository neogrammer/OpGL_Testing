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
    std::vector<VoxelVertex> opaqueVerts;
    std::vector<VoxelVertex> waterVerts;

    opaqueVerts.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6);
    waterVerts.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6);
    //verts.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6);

    for (int z = 0; z < CHUNK_SIZE; z++)
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Block b = c.blocks[Idx(x, y, z)];
                if (!IsSolid(b)) continue;

                int wx = c.coord.x * CHUNK_SIZE + x;
                int wy = c.coord.y * CHUNK_SIZE + y;
                int wz = c.coord.z * CHUNK_SIZE + z;

                glm::vec3 base = glm::vec3(wx, wy, wz);

                glm::vec3 center = base + glm::vec3(0.5f);
                glm::vec3 outDir = glm::normalize(center);

                // pick the dominant axis of outDir (most outward face)
                glm::vec3 a = glm::abs(outDir);

                int topFi;
                if (a.x >= a.y && a.x >= a.z)      topFi = (outDir.x >= 0.0f) ? 0 : 1; // +X or -X
                else if (a.y >= a.x && a.y >= a.z) topFi = (outDir.y >= 0.0f) ? 2 : 3; // +Y or -Y
                else                               topFi = (outDir.z >= 0.0f) ? 4 : 5; // +Z or -Z

                int bottomFi = topFi ^ 1; // works because pairs are (0,1), (2,3), (4,5)

                for (int fi = 0; fi < 6; fi++) {
                    const FaceDef& f = FACES[fi];

                    Block nb = GetBlock(wx + f.dir.x, wy + f.dir.y, wz + f.dir.z);
                    if (!ShouldRenderFace(b, nb)) continue;

                    glm::ivec2 tile;
                    if (fi == topFi)        tile = TILE_TOP;
                    else if (fi == bottomFi) tile = TILE_BOTTOM;
                    else                    tile = SIDE_TILE[fi];

                    glm::vec2 faceUV[4];
                    GetCubeNetUVsTile(fi, tile.x, tile.y, cubeNetW, cubeNetH, faceUV);

                    auto& out = (b == Block::Water) ? waterVerts : opaqueVerts;

                    auto push = [&](int ci) {
                        VoxelVertex v;
                        v.pos = base + glm::vec3(f.c[ci]);
                        v.uv = faceUV[ci];
                        v.normal = f.normal;
                        v.layer = (float)BlockLayer(b);
                        out.push_back(v);
                        };

                    //auto push = [&](int ci) {
                    //    VoxelVertex v;
                    //    v.pos = base + glm::vec3(f.c[ci]);
                    //    v.uv = faceUV[ci];
                    //    v.normal = f.normal;
                    //    verts.push_back(v);
                    //    };


                    // 2 triangles (0,1,2) (0,2,3)
                    push(0); push(1); push(2);
                    push(0); push(2); push(3);


                }
            }
           
    //if (c.vao == 0) glGenVertexArrays(1, &c.vao);
    //if (c.vbo == 0) glGenBuffers(1, &c.vbo);

    //glBindVertexArray(c.vao);
    //glBindBuffer(GL_ARRAY_BUFFER, c.vbo);
    //glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VoxelVertex), verts.data(), GL_STATIC_DRAW);

    //glEnableVertexAttribArray(0);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, pos));
    //glEnableVertexAttribArray(1);
    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, uv));
    //glEnableVertexAttribArray(2);
    //glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, normal));

    //glBindVertexArray(0);

    //c.vertexCount = (int)verts.size();

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
