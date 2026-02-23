#include "../World.h"

#include "../Mesher.h"
#include "../mesh/VoxelVertex.h"
#include <algorithm>


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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, localUV));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, normal));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, layer));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), (void*)offsetof(VoxelVertex, tile));
    glBindVertexArray(0);

    outCount = (int)verts.size();
}

void World::DrawWaterSorted(const glm::vec3& cameraPos)
{
    struct Item { float d2; Chunk* c; };

    std::vector<Item> list;
    list.reserve(chunks.size());

    for (auto& [coord, chunk] : chunks)
    {
        if (chunk.water.count == 0) continue; // or whatever "empty" check you use

        // chunk center in world space (assuming CHUNK_SIZE voxels)
        glm::vec3 center =
            glm::vec3(coord.x, coord.y, coord.z) * float(CHUNK_SIZE) +
            glm::vec3(float(CHUNK_SIZE) * 0.5f);

        glm::vec3 v = center - cameraPos;
        float d2 = glm::dot(v, v);

        list.push_back({ d2, &chunk });
    }

    std::sort(list.begin(), list.end(),
        [](const Item& a, const Item& b) { return a.d2 > b.d2; }); // back-to-front

    for (auto& it : list)
        it.c->water.Draw(); // or whatever your draw call is
}