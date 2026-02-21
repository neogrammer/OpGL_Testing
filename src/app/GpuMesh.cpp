#include "GpuMesh.h"
#include <cstddef> // offsetof
#include "../mesh/VoxelVertex.h"


void GpuMesh::Upload(const std::vector<VoxelVertex>& verts)
{
    if (verts.empty()) {
        count = 0;
        return;
    }

    if (vao == 0) glGenVertexArrays(1, &vao);
    if (vbo == 0) glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(VoxelVertex),
        verts.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
        (void*)offsetof(VoxelVertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
        (void*)offsetof(VoxelVertex, uv));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
        (void*)offsetof(VoxelVertex, normal));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex),
        (void*)offsetof(VoxelVertex, layer));

    glBindVertexArray(0);

    count = (int)verts.size();
}

void GpuMesh::Draw() const
{
    if (count <= 0 || vao == 0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void GpuMesh::Destroy()
{
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    count = 0;
}