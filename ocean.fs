#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
flat in vec3 Normal;     // from vertex, but we’ll recompute anyway
uniform sampler2DArray texArray;
uniform vec2  uTexSize;

// water tile inside the cube-net (same values as ChunkMesher.cpp)
uniform vec2  uWaterTileCR = vec2(1.0, 2.0);  // TILE_TOP (col,row)
uniform float uOceanTileWorld = 8.0;          // world-units per texture repeat
uniform float uOceanAlpha = 0.55;
uniform float uWaterLayer = 5.0;             // layer index of voxel_cube_water.png in your array

vec2 TileUV(vec2 uv, vec2 tileCR)
{
    // cube-net grid is 4x3
    float tileW = 1.0 / 4.0;
    float tileH = 1.0 / 3.0;

    // half-texel padding (matches your atlas-safe sampling)
    float padU = 0.5 / uTexSize.x;
    float padV = 0.5 / uTexSize.y;

    float col = floor(tileCR.x + 0.5);
    float row = floor(tileCR.y + 0.5);

    float u0 = col * tileW + padU;
    float v0 = row * tileH + padV;

    vec2 size = vec2(tileW - 2.0 * padU,
                     tileH - 2.0 * padV);

    vec2 f = fract(uv); // repeat
    return vec2(u0, v0) + f * size;
}

vec3 SampleWaterTile(vec2 uv)
{
    vec2 atlasUV = TileUV(uv, uWaterTileCR);
    return texture(texArray, vec3(atlasUV, uWaterLayer)).rgb;
}

void main()
{
    vec3 n = normalize(WorldPos);
    vec3 an = abs(n);

    // Triplanar weights (sharpen a bit so it’s not mushy)
    vec3 w = pow(an, vec3(4.0));
    w /= (w.x + w.y + w.z + 1e-6);

    float s = 1.0 / max(uOceanTileWorld, 1e-3);

    // axis projections (world space)
    vec2 uvX = WorldPos.zy * s; // X projection uses ZY
    vec2 uvY = WorldPos.xz * s; // Y projection uses XZ
    vec2 uvZ = WorldPos.xy * s; // Z projection uses XY

    vec3 cx = SampleWaterTile(uvX);
    vec3 cy = SampleWaterTile(uvY);
    vec3 cz = SampleWaterTile(uvZ);

    vec3 col = cx * w.x + cy * w.y + cz * w.z;

    FragColor = vec4(col, uOceanAlpha);
}