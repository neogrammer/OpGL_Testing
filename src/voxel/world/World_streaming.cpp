#include "../World.h"
#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>


glm::ivec3 CameraVoxel(glm::vec3 camPos) {
return glm::ivec3((int)std::floor(camPos.x),
    (int)std::floor(camPos.y),
    (int)std::floor(camPos.z));
}

ChunkCoord CameraChunk(glm::vec3 camPos) {
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
            c.opaque.Destroy();
            c.water.Destroy();

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
         
        }



    }
}
