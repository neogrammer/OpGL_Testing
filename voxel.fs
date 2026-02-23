#version 330 core
out vec4 FragColor;

in vec2 LocalUV;
flat in vec2 Tile;
flat in vec3 Normal;
flat in float TexLayer;

uniform sampler2DArray texArray;
uniform vec2 uTexSize; // full texture size of each layer (texW, texH)
uniform vec3 lightDir = normalize(vec3(0.3, 0.8, 0.2));

vec2 CubeNetTiledUV(vec2 localUV, vec2 tileCR, vec3 n)
{
    // cube-net grid is 4x3
    float tileW = 1.0 / 4.0;
    float tileH = 1.0 / 3.0;

    // half-texel padding (matches your C++ padding)
    float padU = 0.5 / uTexSize.x;
    float padV = 0.5 / uTexSize.y;

    float col = floor(tileCR.x + 0.5);
    float row = floor(tileCR.y + 0.5);

    float u0 = col * tileW + padU;
    float v0 = row * tileH + padV;

    vec2 size = vec2(tileW - 2.0 * padU,
                     tileH - 2.0 * padV);

    // Repeat per block across greedy quads:
    vec2 uv = fract(localUV);

    // This replicates your FACE_UV_MAP behavior:
    // - Y faces swap
    // - X/Z faces flip U
    bool yFace = abs(n.y) > 0.5;
    if (yFace) {
        uv = uv.yx;
    } else {
        uv.x = 1.0 - uv.x;
    }

    return vec2(u0, v0) + uv * size;
}

void main() {
    vec2 uv = CubeNetTiledUV(LocalUV, Tile, Normal);

    vec4 tex = texture(texArray, vec3(uv, TexLayer));

    float ndl = max(dot(normalize(Normal), normalize(lightDir)), 0.0);
    float ambient = 0.25;
    vec3 lit = tex.rgb * (ambient + ndl * 0.75);

    // If you want real water transparency, switch alpha back to tex.a
    FragColor = vec4(lit, 1.0);
}