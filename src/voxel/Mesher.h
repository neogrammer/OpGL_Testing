#pragma once
#include <glm.hpp>

struct FaceDef {
    glm::ivec3 dir;
    glm::vec3 normal;
    glm::ivec3 c[4]; // corners (0 or 1 offsets)
};

// FACES order must match your mesher's faces:
// 0:+X, 1:-X, 2:+Y, 3:-Y, 4:+Z, 5:-Z
static const glm::ivec2 FACE_TILE[6] = {
    {2,1}, // +X (right)
    {0,1}, // -X (left)
    {1,2}, // +Y (top)
    {1,0}, // -Y (bottom)
    {1,1}, // +Z (front)
    {3,1}, // -Z (back)
};

// UV corner mapping to avoid mirroring based on our corner ordering.
// Corners are in the same order as your FACES[i].c[0..3].
static const int FACE_UV_MAP[6][4] = {
    {3,2,1,0}, // +X
    {3,2,1,0}, // -X
    {0,3,2,1}, // +Y
    {0,3,2,1}, // -Y
    {3,2,1,0}, // +Z
    {3,2,1,0}, // -Z
};

// texW/texH are the full texture dimensions (128x96 here).
// padding is half-texel to avoid accidental edge sampling.
inline void GetCubeNetUVs(int faceIndex, int texW, int texH, glm::vec2 outUV[4]) {
    const float tileW = 1.0f / 4.0f;
    const float tileH = 1.0f / 3.0f;

    int col = FACE_TILE[faceIndex].x;
    int row = FACE_TILE[faceIndex].y;
    //float flippedRow = 2.f - row;   // since 3 rows total (0,1,2)



    float u0 = col * tileW;
   // float v0 = flippedRow * tileH;
    float u1 = u0 + tileW;
    //float v1 = v0 + tileH;
    float v0 = row * tileH;
    float v1 = v0 + tileH;
    // half texel padding (safe even with NEAREST, useful if you later change filters)
    float padU = 0.5f / float(texW);
    float padV = 0.5f / float(texH);
    u0 += padU; v0 += padV;
    u1 -= padU; v1 -= padV;

    // rect corners in the common order:
    // 0:(u0,v0) 1:(u0,v1) 2:(u1,v1) 3:(u1,v0)
    glm::vec2 rect[4] = {
        {u0,v0}, {u0,v1}, {u1,v1}, {u1,v0}
    };

    for (int c = 0; c < 4; c++) {
        outUV[c] = rect[FACE_UV_MAP[faceIndex][c]];
    }
}


inline void GetCubeNetUVsTile(int faceIndex, int col, int row, int texW, int texH, glm::vec2 outUV[4]) {
    const float tileW = 1.0f / 4.0f;
    const float tileH = 1.0f / 3.0f;

    // you used stbi_set_flip_vertically_on_load(true)
    //int flippedRow = 2 - row; // rows: 0,1,2

    float u0 = col * tileW;
    //float v0 = flippedRow * tileH;
    float u1 = u0 + tileW;
   // float v1 = v0 + tileH;

    float v0 = row * tileH;
    float v1 = v0 + tileH;

    // half-texel padding (prevents bleeding between tiles)
    float padU = 0.5f / float(texW);
    float padV = 0.5f / float(texH);
    u0 += padU; v0 += padV;
    u1 -= padU; v1 -= padV;

    glm::vec2 rect[4] = {
        {u0,v0}, {u0,v1}, {u1,v1}, {u1,v0}
    };

    for (int c = 0; c < 4; c++) {
        outUV[c] = rect[FACE_UV_MAP[faceIndex][c]];
    }
}

    static const FaceDef FACES[6] = {
    {{ 1,0,0}, { 1,0,0}, {{1,0,0},{1,1,0},{1,1,1},{1,0,1}}}, // +X
    {{-1,0,0}, {-1,0,0}, {{0,0,1},{0,1,1},{0,1,0},{0,0,0}}}, // -X
    {{0, 1,0}, {0, 1,0}, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}}}, // +Y
    {{0,-1,0}, {0,-1,0}, {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}}, // -Y
    {{0,0, 1}, {0,0, 1}, {{1,0,1},{1,1,1},{0,1,1},{0,0,1}}}, // +Z
    {{0,0,-1}, {0,0,-1}, {{0,0,0},{0,1,0},{1,1,0},{1,0,0}}}, // -Z
};

// basic per-face UV (full tile)
static const glm::vec2 UV4[4] = {
    {0,0},{0,1},{1,1},{1,0}
};
