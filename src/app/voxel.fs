#version 330 core
out vec4 FragColor;

in vec2 LocalUV;
flat in vec2 Tile;
flat in vec3 Normal;
flat in float TexLayer;

// must be output by voxel.vs
in vec3 WorldPos;

uniform sampler2DArray texArray;
uniform vec2 uTexSize;

uniform vec3  uCameraPos;

// Lighting (set from C++)
// Define uLightDir as: direction FROM surface TOWARD the sun (world-space)
uniform vec3  uLightDir;
uniform float uAmbient;

// Toon / cel shading controls (set from C++)
uniform bool  uToon;
uniform float uToonSteps;
uniform float uRimStrength;
uniform float uRimPower;
uniform float uGridLineStrength;
uniform float uGridLineWidth;

// Water / underwater uniforms
uniform bool  uWaterPass;
uniform float uSeaR;

uniform float uWaterAbsorb;
uniform float uFresnelBoost;

uniform vec3  uUnderTintColor;
uniform float uShallowDepth;
uniform float uDeepDepth;
uniform float uShallowAlpha;
uniform float uDeepAlpha;

// Optional fog (set from C++). Safe if you don't set them because we guard.
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3  uFogColor;

vec2 CubeNetTiledUV(vec2 localUV, vec2 tileCR, vec3 n)
{
    float tileW = 1.0 / 4.0;
    float tileH = 1.0 / 3.0;

    float padU = 0.5 / uTexSize.x;
    float padV = 0.5 / uTexSize.y;

    float col = floor(tileCR.x + 0.5);
    float row = floor(tileCR.y + 0.5);

    float u0 = col * tileW + padU;
    float v0 = row * tileH + padV;

    vec2 size = vec2(tileW - 2.0 * padU,
                     tileH - 2.0 * padV);

    vec2 uv = fract(localUV);

    bool yFace = abs(n.y) > 0.5;
    if (yFace) {
        uv = uv.yx;
    } else {
        uv.x = 1.0 - uv.x;
    }

    return vec2(u0, v0) + uv * size;
}

void main()
{
    vec2 uv = CubeNetTiledUV(LocalUV, Tile, Normal);
    vec4 tex = texture(texArray, vec3(uv, TexLayer));

    vec3 color = tex.rgb;
    float outA = 1.0;

    // --- Underwater detection / depth ---
    float rFrag = length(WorldPos);
    float rCam  = length(uCameraPos);

    bool cameraUnder = (uSeaR > 0.0) && (rCam  < uSeaR);
    bool fragUnder   = (uSeaR > 0.0) && (rFrag < uSeaR);
    bool underwater  = cameraUnder || fragUnder;

    // Depth below sea surface.
    // For water faces, sample half a voxel inside in the radial direction (reduces rings).
    float rForDepth = rFrag;
    if (uWaterPass) {
        vec3 outDir = (rFrag > 1e-5) ? (WorldPos / rFrag) : vec3(0.0, 1.0, 0.0);
        rForDepth = length(WorldPos - outDir * 0.5);
    }
    float depth = max(uSeaR - rForDepth, 0.0);

    float t = 0.0;
    if (uDeepDepth > uShallowDepth)
        t = clamp((depth - uShallowDepth) / (uDeepDepth - uShallowDepth), 0.0, 1.0);

    // --- Water vs Opaque ---
    if (uWaterPass)
    {
        vec3 shallowCol = vec3(0.18, 0.45, 0.95);
        vec3 deepCol    = vec3(0.02, 0.10, 0.35);

        vec3 waterCol = mix(shallowCol, deepCol, t);
        color = mix(color, waterCol, 0.55);

        outA = clamp(mix(uShallowAlpha, uDeepAlpha, t), 0.0, 0.98);
    }
    else
    {
        // Diffuse lighting ONLY for non-water voxels
        vec3 N = normalize(Normal);
        vec3 L = normalize(uLightDir);     // <-- IMPORTANT: no minus. Fixes upside-down lighting.

        float diff = max(dot(N, L), 0.0);
        float shade = clamp(uAmbient + diff * (1.0 - uAmbient), 0.0, 1.0);

        // Cel shading: quantize lighting + optional rim + voxel grid lines.
        if (uToon)
        {
            float steps = max(uToonSteps, 1.0);
            shade = floor(shade * steps) / steps;

            // Rim darkening (cheap outline-ish look)
            vec3 V = normalize(uCameraPos - WorldPos);
            float rim = 1.0 - max(dot(N, V), 0.0);
            rim = pow(rim, max(uRimPower, 0.001));
            shade = mix(shade, shade * 0.35, clamp(rim * uRimStrength, 0.0, 1.0));

            // Lines at voxel boundaries even on greedy-merged quads.
            vec2 cell = fract(LocalUV);
            float border = min(min(cell.x, 1.0 - cell.x), min(cell.y, 1.0 - cell.y));
            float aa = fwidth(border);
            float line = 1.0 - smoothstep(uGridLineWidth, uGridLineWidth + aa, border);
            shade = mix(shade, shade * 0.15, clamp(line * uGridLineStrength, 0.0, 1.0));
        }

        color *= shade;
    }

    // --- Underwater tint (applies to both terrain + water) ---
    if (underwater)
    {
        float tintT = clamp(depth / max(uDeepDepth, 0.001), 0.0, 1.0);
        float strength = mix(0.20, 0.45, tintT);
        color = mix(color, color * uUnderTintColor, strength);
    }

    // --- Fog (optional) ---
    if (uFogEnd > uFogStart)
    {
        float dist = length(uCameraPos - WorldPos);
        float fog = smoothstep(uFogStart, uFogEnd, dist);
        color = mix(color, uFogColor, fog);
    }

    FragColor = vec4(color, outA);
}