#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec2 aLocalUV;   // 0..w/h in "block units"
layout (location=2) in vec3 aNormal;
layout (location=3) in float aLayer;
layout (location=4) in vec2 aTile;      // (col,row) in the cube-net grid

out vec2 LocalUV;
flat out vec2 Tile;
flat out vec3 Normal;
flat out float TexLayer;
out vec3 WorldPos;
uniform mat4 view;
uniform mat4 projection;

void main() {
    LocalUV = aLocalUV;
    Tile = aTile;
    Normal = aNormal;
    TexLayer = aLayer;
    WorldPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}