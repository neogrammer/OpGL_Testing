#include "../World.h"

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