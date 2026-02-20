#include "World.h"
#include "Mesher.h"
#include <glad/glad.h>
#include <glm.hpp>

struct VoxelVertex { glm::vec3 pos; glm::vec2 uv; glm::vec3 normal; };

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
    if (it != chunks.end()) {
        return it->second.blocks[Idx(lx, ly, lz)];
    }

    // procedural fallback (seamless borders)
    glm::vec3 p = glm::vec3(wx, wy, wz) + glm::vec3(0.5f);
    return SamplePlanet(p, planet);
}

void World::FillChunkBlocks(Chunk& c) {
    for (int z = 0; z < CHUNK_SIZE; z++)
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                int wx = c.coord.x * CHUNK_SIZE + x;
                int wy = c.coord.y * CHUNK_SIZE + y;
                int wz = c.coord.z * CHUNK_SIZE + z;

                glm::vec3 p = glm::vec3(wx, wy, wz) + glm::vec3(0.5f);
                c.blocks[Idx(x, y, z)] = SamplePlanet(p, planet);
            }
    c.dirty = true;
}

void World::BuildChunkMesh(Chunk& c) {
    std::vector<VoxelVertex> verts;
    verts.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6);

    for (int z = 0; z < CHUNK_SIZE; z++)
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Block b = c.blocks[Idx(x, y, z)];
                if (!IsSolid(b)) continue;

                int wx = c.coord.x * CHUNK_SIZE + x;
                int wy = c.coord.y * CHUNK_SIZE + y;
                int wz = c.coord.z * CHUNK_SIZE + z;

                glm::vec3 base = glm::vec3(wx, wy, wz);

                for (int fi = 0; fi < 6; fi++) {
                    const FaceDef& f = FACES[fi];

                    Block nb = GetBlock(wx + f.dir.x, wy + f.dir.y, wz + f.dir.z);
                    if (IsSolid(nb)) continue;
                    glm::vec2 faceUV[4];
                    GetCubeNetUVs(fi, 128, 96, faceUV);  // use actual loaded texture size

                    auto push = [&](int ci) {
                        VoxelVertex v;
                        v.pos = base + glm::vec3(f.c[ci]);
                        v.uv = faceUV[ci];
                        v.normal = f.normal;
                        verts.push_back(v);
                        };
                  

                    // 2 triangles (0,1,2) (0,2,3)
                    push(0); push(1); push(2);
                    push(0); push(2); push(3);
                }
            }

    if (c.vao == 0) glGenVertexArrays(1, &c.vao);
    if (c.vbo == 0) glGenBuffers(1, &c.vbo);

    glBindVertexArray(c.vao);
    glBindBuffer(GL_ARRAY_BUFFER, c.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VoxelVertex), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, normal));

    glBindVertexArray(0);

    c.vertexCount = (int)verts.size();
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

void World::Draw() const {
    for (auto& [cc, c] : chunks) {
        if (c.vertexCount == 0) continue;
        glBindVertexArray(c.vao);
        glDrawArrays(GL_TRIANGLES, 0, c.vertexCount);
    }
    glBindVertexArray(0);
}