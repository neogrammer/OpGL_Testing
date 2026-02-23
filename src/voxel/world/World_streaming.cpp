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


struct Candidate { ChunkCoord cc; int dist2; float score; };

static float ScoreChunkFrontFirst(const ChunkCoord& cc,
    const ChunkCoord& camCC,
    const glm::vec3& camFwdNorm,
    float frontBias)
{
    glm::vec3 off = glm::vec3(cc.x - camCC.x, cc.y - camCC.y, cc.z - camCC.z);

    float dist2 = glm::dot(off, off);

    float d = 0.0f;
    float len = glm::length(off);
    if (len > 0.0001f)
        d = glm::dot(off / len, camFwdNorm); // [-1..1], higher = more in front

    // lower score = higher priority
    return dist2 - d * frontBias;
}

static bool PopBestChunk(std::deque<ChunkCoord>& q,
    const ChunkCoord& camCC,
    const glm::vec3& camFwdNorm,
    float frontBias,
    ChunkCoord& out)
{
    if (q.empty()) return false;

    auto bestIt = q.begin();
    float bestScore = ScoreChunkFrontFirst(*bestIt, camCC, camFwdNorm, frontBias);

    for (auto it = std::next(q.begin()); it != q.end(); ++it)
    {
        float s = ScoreChunkFrontFirst(*it, camCC, camFwdNorm, frontBias);
        if (s < bestScore)
        {
            bestScore = s;
            bestIt = it;
        }
    }

    out = *bestIt;
    q.erase(bestIt);
    return true;
}

void World::UpdateStreaming(glm::vec3 cameraPos, glm::vec3 cameraForward)
{
    ChunkCoord cc = CameraChunk(cameraPos);

    streamCamChunk = cc;

    float fLen = glm::length(cameraForward);
    streamCamForward = (fLen > 0.0001f) ? (cameraForward / fLen) : glm::vec3(0, 0, -1);

    std::vector<Candidate> cand;
    int side = 2 * renderDistance + 1;
    cand.reserve(size_t(side) * side * side);
    for (int dz = -renderDistance; dz <= renderDistance; dz++)
        for (int dy = -renderDistance; dy <= renderDistance; dy++)
            for (int dx = -renderDistance; dx <= renderDistance; dx++)
            {
                ChunkCoord want{ cc.x + dx, cc.y + dy, cc.z + dz };
                int dist2 = dx * dx + dy * dy + dz * dz;
                float score = ScoreChunkFrontFirst(want, cc, streamCamForward, streamFrontBias);
                cand.emplace_back(Candidate{ (ChunkCoord)want, (int)dist2, (float)score });
                // or: cand.emplace_back(Candidate{ want, dist2, score });

            }

    std::sort(cand.begin(), cand.end(),
        [](const Candidate& a, const Candidate& b) { return a.score < b.score; });
    for (auto& want : cand)
    {
        if (chunks.find(want.cc) == chunks.end())
        {
            // insert the empty chunk now so we dont enqueue duplicates
            Chunk c; c.coord = want.cc;
            c.queuedGen = true;
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
        ChunkCoord cc;
        if (!PopBestChunk(genQueue, streamCamChunk, streamCamForward, streamFrontBias, cc))
            break;

        auto it = chunks.find(cc);
        if (it == chunks.end()) continue;

        Chunk& c = it->second;
        c.queuedGen = false;

        static const ChunkCoord N6[6] = {
    { 1,0,0}, {-1,0,0},
    { 0,1,0}, { 0,-1,0},
    { 0,0,1}, { 0,0,-1}
        };


        FillChunkBlocks(c);
        if (!c.queuedMesh) {
            c.queuedMesh = true;
            meshQueue.push_back(cc);
        }

        // neighbors
        for (auto d : N6) {
            ChunkCoord n{ cc.x + d.x, cc.y + d.y, cc.z + d.z };
            auto itN = chunks.find(n);
            if (itN != chunks.end() && itN->second.generated) {
                Chunk& cn = itN->second;
                if (cn.generated && !cn.queuedMesh) {
                    cn.queuedMesh = true;
                    meshQueue.push_back(n);
                }
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
        ChunkCoord cc;
        if (!PopBestChunk(meshQueue, streamCamChunk, streamCamForward, streamFrontBias, cc))
            break;

        auto it = chunks.find(cc);
        if (it == chunks.end()) continue;

        Chunk& c = it->second;
        c.queuedMesh = false;
        BuildChunkMesh(c);

        static double t0 = glfwGetTime();
        double t = glfwGetTime();
        if (t - t0 > 1.0) {
            t0 = t;
         
        }



    }
}
