#version 330 core
out vec4 FragColor;

in vec2 LocalUV;
flat in vec2 Tile;
flat in vec3 Normal;
flat in float TexLayer;

// ADDED (must be output by voxel.vs)
in vec3 WorldPos;

uniform sampler2DArray texArray;
uniform vec2 uTexSize; // full texture size of each layer (texW, texH)
uniform vec3 lightDir = normalize(vec3(0.3, 0.8, 0.2));
uniform vec3  uCameraPos;
uniform float uWaterAbsorb = 0.015;   // start here
uniform float uFresnelBoost = 0.35;   // start here

// Water / underwater uniforms
uniform bool  uWaterPass = false;
uniform float uSeaR = 0.0;

uniform vec3  uUnderTintColor = vec3(0.25, 0.45, 1.0);

// (optional knobs you can set from C++)
uniform float uShallowDepth = 6.0;
uniform float uDeepDepth    = 40.0;
uniform float uShallowAlpha = 0.35;
uniform float uDeepAlpha    = 0.80;



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

    // Replicate FACE_UV_MAP behavior:
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

    //float ndl = max(dot(normalize(Normal), normalize(lightDir)), 0.0);
    //float ambient = 0.25;
    //vec3 lit = tex.rgb * (ambient + ndl * 0.75);
    vec3 lit = tex.rgb; // ambient-only lighting (uniform)

    //// --- Underwater tint & water appearance ---
    //float r = length(WorldPos);
    //bool underwater = (uSeaR > 0.0) && (r < uSeaR);

    //// depth below sea surface in world units
    //float depth = max(uSeaR - r, 0.0);
//    float rFrag = length(WorldPos);
//bool underwater = (uSeaR > 0.0) && (rFrag < uSeaR);

//// For water shading, sample half a voxel INSIDE the water block so the whole face
//// behaves consistently (prevents the “half above/half below” ring look).
//float rForDepth = rFrag;
//if (uWaterPass) {
//    rForDepth = length(WorldPos - Normal * 0.5);
//}

//float depth = max(uSeaR - rForDepth, 0.0);

float rFrag = length(WorldPos);
float rCam  = length(uCameraPos);

bool cameraUnder = (uSeaR > 0.0) && (rCam  < uSeaR);
bool fragUnder   = (uSeaR > 0.0) && (rFrag < uSeaR);

// When the camera is underwater, keep shading consistent (prevents rings).
bool underwater = cameraUnder || fragUnder;

// Depth below sea surface.
// For water, sample half a voxel INSIDE the block in the RADIAL direction.
float rForDepth = rFrag;
if (uWaterPass) {
    vec3 outDir = (rFrag > 1e-5) ? (WorldPos / rFrag) : vec3(0.0, 1.0, 0.0);
    rForDepth = length(WorldPos - outDir * 0.5);
}

float depth = max(uSeaR - rForDepth, 0.0);

    // 0 shallow -> 1 deep
    float t = 0.0;
    if (uDeepDepth > uShallowDepth)
        t = clamp((depth - uShallowDepth) / (uDeepDepth - uShallowDepth), 0.0, 1.0);

    // Tint anything underwater (terrain + water)
    if (underwater)
    {
        // stronger tint as you go deeper
        float tintT = clamp(depth / max(uDeepDepth, 0.001), 0.0, 1.0);
        float strength = mix(0.20, 0.45, tintT);
        lit = mix(lit, lit * uUnderTintColor, strength);
    }

    if (uWaterPass)
    {
//        if (!gl_FrontFacing) {
//    vec3 outDir = normalize(WorldPos);
//    float align = abs(dot(normalize(Normal), outDir)); // 1 = radial-ish, 0 = tangent-ish

//    // Radial faces are the "real surface" we want visible from below.
//    // Tangent faces are step-walls we want to hide.
//    float keep = smoothstep(0.55, 0.80, align);

//    if (keep < 0.02) discard;
//    // or: outA *= keep; (if you prefer fade instead of discard)
//}

        // Water color: shallow brighter, deep darker
        vec3 shallowCol = vec3(0.18, 0.45, 0.95);
        vec3 deepCol    = vec3(0.02, 0.10, 0.35);

        vec3 waterCol = mix(shallowCol, deepCol, t);
        lit = mix(lit, waterCol, 0.55);

        // Water opacity: shallow clearer, deep darker
        float outA = mix(uShallowAlpha, uDeepAlpha, t);

        vec3 V = normalize(uCameraPos - WorldPos);
//float ndv = clamp(dot(normalize(Normal), V), 0.0, 1.0);
//float fres = pow(1.0 - ndv, 5.0);   // 0 front-on, 1 grazing

//float dist = length(uCameraPos - WorldPos);
//float absorb = 1.0 - exp(-uWaterAbsorb * dist);

//// existing outA from shallow/deep
//outA = clamp(outA + absorb * 0.70 + fres * uFresnelBoost, 0.0, 0.98);

outA = clamp(outA, 0.0, 0.98);

        FragColor = vec4(lit, outA);
    }
    else
    {
        FragColor = vec4(lit, 1.0);
    }
}