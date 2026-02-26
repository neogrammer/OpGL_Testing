#include "App.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <src/utility/TextureUtils.h>

bool App::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width_, height_, "VoxelPlanet", nullptr, nullptr);
    if (!window_) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
   
    glfwSetWindowAspectRatio(window_, INIT_W, INIT_H);
    glfwSetWindowPos(window_, 150, 50);

    glfwSetWindowUserPointer(window_, this);

    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* w, int a, int b) {
        static_cast<App*>(glfwGetWindowUserPointer(w))->OnResize(a, b);
        });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double x, double y) {
        static_cast<App*>(glfwGetWindowUserPointer(w))->OnMouse(x, y);
        });

    glfwSetScrollCallback(window_, [](GLFWwindow* w, double xoff, double yoff) {
        static_cast<App*>(glfwGetWindowUserPointer(w))->OnScroll(xoff, yoff);
        });

    return true;
}

bool App::InitGL()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return false;
    }
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    return true;
}

bool App::LoadAssets()
{
    voxelShader_ = std::make_unique<Shader>("voxel.vs", "voxel.fs");
    oceanShader_ = std::make_unique<Shader>("voxel.vs", "ocean.fs");
    world_.planet.baseRadius = 4096.f;
    world_.planet.maxHeight = 12.0f;
    world_.planet.noiseFreq = 3.0f;
    world_.planet.octaves = 5;
    world_.planet.seaLevelOffset = -2.0f;

    blockTexArray_ = util::LoadTexture2DArray({
        "assets/textures/voxel_cube_grass.png",
        "assets/textures/voxel_cube_dirt.png",
        "assets/textures/voxel_cube_stone_2.png",
        "assets/textures/voxel_cube_sand.png",
        "assets/textures/voxel_cube_snow.png",
        "assets/textures/voxel_cube_water.png",
        }, texW_, texH_);

    voxelShader_->use();
    voxelShader_->setInt("texArray", 0);
    voxelShader_->setVec2("uTexSize", (float)texW_, (float)texH_);

    // --- Simple directional sun lighting (for non-water voxels) ---
    voxelShader_->setVec3("uLightDir", glm::normalize(glm::vec3(0.35f, 1.0f, 0.25f))); // world-space
    voxelShader_->setFloat("uAmbient", 0.22f); // tweak

    return true;
}

int App::Run()
{
    if (!InitWindow()) return -1;
    if (!InitGL()) return -1;
    if (!LoadAssets()) return -1;

    // Start somewhere above the planet, then snap down to tiny-player eye height.
    camera_.Position = glm::vec3(0.0f, 0.0f, world_.planet.baseRadius + 20.0f);
    SnapCameraToSurface(spawnAboveSea_);
    InitPlayerFromCamera();


    glClearColor(.16f, .46f, 96.f, 1.0f);


    //world_.BuildPlanetOnce();
    while (!glfwWindowShouldClose(window_))
    {
        float current = (float)glfwGetTime();
        deltaTime_ = current - lastFrame_;
        lastFrame_ = current;
        
        // Lock camera "up" to planet radial up (prevents roll)
        camera_.SetWorldUp(glm::normalize(camera_.Position));
        ProcessInput();

        // FPS mode = voxel collisions + gravity + jumping
        if (!camera_.IsFlyMode())
            UpdatePlayerPhysics(deltaTime_);

        camera_.SetWorldUp(glm::normalize(camera_.Position));

        world_.UpdateStreaming(camera_.Position, camera_.Front);

        if (loading_)
        {
            world_.TickBuildQueues(loadGenPerFrame_, loadMeshPerFrame_);

            World::StreamStats st = world_.GetStreamStats();
            float p = (st.target > 0) ? (float(st.meshed) / float(st.target)) : 1.0f;
            int pct = int(p * 100.0f + 0.5f);

            double t = glfwGetTime();
            if (t - loadingTitleT0_ > 0.10)
            {
                char title[256];
                std::snprintf(title, sizeof(title),"VoxelPlanet - Loading %d%% (meshed %zu/%zu) [genQ=%zu meshQ=%zu]",  pct, st.meshed, st.target, st.genQ, st.meshQ);
                glfwSetWindowTitle(window_, title);
                loadingTitleT0_ = t;
            }

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffers(window_);
            glfwPollEvents();

            if (world_.IsStreamReady())
            {
                loading_ = false;
                glfwSetWindowTitle(window_, "VoxelPlanet");
                glClearColor(.16f, .46f, 96.f, 1.0f); // your original
            }

            continue; // IMPORTANT: skip normal rendering until ready
        }
        world_.TickBuildQueues(playGenPerFrame_, playMeshPerFrame_);
        //world_.TickBuildQueues(2,4);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera_.Zoom),
            (float)width_ / (float)height_, 0.03f, 2000.0f);

        glm::mat4 view = camera_.GetViewMatrix();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, blockTexArray_);

       
        voxelShader_->use();
        voxelShader_->setMat4("projection", projection);
        voxelShader_->setMat4("view", view);

        voxelShader_->setVec3("uCameraPos", camera_.Position);
        
        glm::vec3 sunDir = glm::normalize(camera_.Position); // outward radial = "up" locally
        voxelShader_->setVec3("uLightDir", sunDir);
        voxelShader_->setFloat("uAmbient", 0.22f);

        // Toon / cel shading (voxel.fs)
        voxelShader_->setBool("uToon", true);
        voxelShader_->setFloat("uToonSteps", 4.0f);
        voxelShader_->setFloat("uRimStrength", 0.35f);
        voxelShader_->setFloat("uRimPower", 2.5f);
        voxelShader_->setFloat("uGridLineStrength", 0.35f);
        voxelShader_->setFloat("uGridLineWidth", 0.06f);

        // --- Fog (end fog slightly before the last visible chunk ring) ---
        const float chunkW = float(CHUNK_SIZE);
        const float SQRT3 = 1.7320508f;

        // If you want fog to finish ~half a chunk before the edge, use -0.5f.
        // Your earlier snippet used -1.0f; both are fine, this one hides pop-in better.
        float fogEnd = float(world_.GetRenderDistance() - 0.5f) * chunkW * SQRT3;
        float fogStart = (fogEnd - 2.0f * chunkW * SQRT3) - 2.f; // fade across ~2 chunks

        voxelShader_->setFloat("uFogStart", fogStart);
        voxelShader_->setFloat("uFogEnd", fogEnd);
        voxelShader_->setVec3("uFogColor", glm::vec3(0.16f, 0.46f, 1.0f)); // match your sky
        
        voxelShader_->setFloat("uWaterAbsorb", 0.015f);
        voxelShader_->setFloat("uFresnelBoost", 0.35f);
       // glDisable(GL_BLEND);
       //glDepthMask(GL_TRUE);
        // sea radius in world units (same units as WorldPos/aPos)
        float seaR = world_.planet.baseRadius + world_.planet.seaLevelOffset;
        voxelShader_->setFloat("uSeaR", seaR);
        voxelShader_->setBool("uWaterPass", false);
        // optional knobs (you can tweak later)
        voxelShader_->setVec3("uUnderTintColor", glm::vec3(0.25f, 0.45f, 1.0f));
        voxelShader_->setFloat("uShallowDepth", 6.0f);
        voxelShader_->setFloat("uDeepDepth", 40.0f);
        voxelShader_->setFloat("uShallowAlpha", 0.35f);
        voxelShader_->setFloat("uDeepAlpha", 0.80f);

        // Fog: fade out ~0.5 chunk before the streaming boundary to hide pop-in
       fogEnd = (world_.GetRenderDistance() - 0.5f) * float(CHUNK_SIZE);
        fogStart = (world_.GetRenderDistance() - 1.5f) * float(CHUNK_SIZE);
        voxelShader_->setFloat("uFogStart", fogStart);
        voxelShader_->setFloat("uFogEnd", fogEnd);

        world_.DrawOpaque();

        

        // Water pass
        //voxelShader_->setBool("uWaterPass", true);

        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //// see water from below
        //glDisable(GL_CULL_FACE);

        //// correct transparency
        //glDepthMask(GL_FALSE);

        //// IMPORTANT: draw water back-to-front (see next patch)
        //world_.DrawWaterSorted(camera_.Position);

        //glDepthMask(GL_TRUE);
        //glEnable(GL_CULL_FACE);
        //glDisable(GL_BLEND);

        //// Water pass
        //voxelShader_->setBool("uWaterPass", true);

        //// Two-pass "single-layer water": prevents stacked water faces from accumulating
        //glDisable(GL_CULL_FACE);

        //// 1) Depth prepass: write nearest water depth only
        //glDisable(GL_BLEND);
        //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        //glDepthMask(GL_TRUE);
        //glDepthFunc(GL_LESS); // default
        //world_.DrawWaterSorted(camera_.Position);

        //// 2) Color pass: only draw fragments that match the nearest depth
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        //glDepthMask(GL_FALSE);
        //glDepthFunc(GL_LEQUAL); // if water disappears on some GPUs, try GL_LEQUAL
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //world_.DrawWaterSorted(camera_.Position);

        //// Restore state
        //glDepthFunc(GL_LESS);
        //glDepthMask(GL_TRUE);
        //glEnable(GL_CULL_FACE);
        //glDisable(GL_BLEND);

        // --- Ocean surface (radial shell) ---
        
        if (!oceanBuilt_) BuildOceanMesh(seaR, 96, 192);

        oceanShader_->use();
        oceanShader_->setMat4("projection", projection);
        oceanShader_->setMat4("view", view);
        oceanShader_->setVec2("uTexSize", (float)texW_, (float)texH_);
        oceanShader_->setFloat("uOceanTileWorld", 8.0f); // tweak: bigger = less repeating
        oceanShader_->setFloat("uOceanAlpha", 0.55f);
        oceanShader_->setFloat("uWaterLayer", 5.0f);

        // same texture array bound already
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-.5f, -.5f);
        oceanMesh_.Draw();
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);


        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
void App::BuildOceanMesh(float R, int stacks, int slices)
{
    std::vector<VoxelVertex> v;
    v.reserve(stacks * slices * 6);

    auto pushTri = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2)
        {
            auto mk = [&](glm::vec3 p)
                {
                    VoxelVertex vv{};
                    vv.pos = p;
                    vv.normal = glm::normalize(p);
                    vv.localUV = glm::vec2(0.0f);         // unused by ocean.fs
                    vv.layer = 5.0f;                      // water layer in your array (index 5)
                    vv.tile = glm::vec2(1.0f, 2.0f);     // TILE_TOP (col,row)
                    return vv;
                };
            v.push_back(mk(p0));
            v.push_back(mk(p1));
            v.push_back(mk(p2));
        };

    for (int i = 0; i < stacks; ++i)
    {
        float v0 = (float)i / (float)stacks;
        float v1 = (float)(i + 1) / (float)stacks;

        float th0 = v0 * glm::pi<float>();
        float th1 = v1 * glm::pi<float>();

        for (int j = 0; j < slices; ++j)
        {
            float u0 = (float)j / (float)slices;
            float u1 = (float)(j + 1) / (float)slices;

            float ph0 = u0 * glm::two_pi<float>();
            float ph1 = u1 * glm::two_pi<float>();

            auto sph = [&](float th, float ph)
                {
                    float x = sin(th) * cos(ph);
                    float y = cos(th);
                    float z = sin(th) * sin(ph);
                    return glm::vec3(x, y, z) * R;
                };

            glm::vec3 p00 = sph(th0, ph0);
            glm::vec3 p10 = sph(th1, ph0);
            glm::vec3 p01 = sph(th0, ph1);
            glm::vec3 p11 = sph(th1, ph1);

            // two triangles
            pushTri(p00, p10, p11);
            pushTri(p00, p11, p01);
        }
    }

    oceanMesh_.Upload(v);
    oceanBuilt_ = true;
}

void App::OnResize(int w, int h)
{
    width_ = std::max(1, w);
    height_ = std::max(1, h);
    glViewport(0, 0, width_, height_);
}

void App::OnMouse(double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse_) {
        lastX_ = xpos;
        lastY_ = ypos;
        firstMouse_ = false;
    }

    float xoffset = xpos - lastX_;
    float yoffset = lastY_ - ypos;

    lastX_ = xpos;
    lastY_ = ypos;

    if (mouseCap_ && !capSpot_) {
        camera_.ProcessMouseMovement(xoffset, yoffset);
    }

    if (capSpot_) {
        capSpot_ = false;
        savedX_ = xposIn;
        savedY_ = yposIn;
    }
}

void App::OnScroll(double, double yoffset)
{
    camera_.ProcessMouseScroll((float)yoffset);
}

float App::ApplyDeadzone(float v, float dz)
{
    const float av = std::fabs(v);
    if (av < dz) return 0.0f;

    // Remap so movement starts smoothly after the deadzone.
    const float sign = (v < 0.0f) ? -1.0f : 1.0f;
    float scaled = (av - dz) / (1.0f - dz);
    if (scaled > 1.0f) scaled = 1.0f;
    return scaled * sign;
}

void App::ToggleFlyMode()
{
    camera_.ToggleFlyMode();

    // When dropping back to FPS mode, snap to the terrain surface at a tiny-player eye height.
    if (!camera_.IsFlyMode())
    {
        SnapCameraToSurface(false);
        InitPlayerFromCamera();
    }

    std::cout << (camera_.IsFlyMode() ? "[Camera] Fly mode ON\n" : "[Camera] FPS mode ON\n");
}

void App::SnapCameraToSurface(bool keepAboveSea)
{
    float len = glm::length(camera_.Position);
    if (len < 1e-5f) return;

    glm::vec3 dir = camera_.Position / len;

    // Terrain surface radius at this direction (continuous surface, matches SamplePlanet height).
    float surfaceR = world_.planet.baseRadius + HeightOnSphere(dir, world_.planet);

    float baseR = surfaceR;
    if (keepAboveSea)
    {
        float seaR = world_.planet.baseRadius + world_.planet.seaLevelOffset;
        baseR = std::max(baseR, seaR);
    }

    camera_.Position = dir * (baseR + playerEyeHeight_);
}

// -----------------------------------------------------------------------------
// Player physics + voxel collisions (FPS mode)
// -----------------------------------------------------------------------------

static inline bool IsCollidableBlock(Block b)
{
    // Treat water as non-solid for now (you can add swimming later).
    return b != Block::Air && b != Block::Water;
}

void App::InitPlayerFromCamera()
{
    glm::vec3 up = glm::normalize(camera_.Position);
    if (glm::length(up) < 1e-6f) up = glm::vec3(0, 1, 0);

    // Store feet position so the physics can drive the camera.
    playerFeetPos_ = camera_.Position - up * playerEyeHeight_;
    playerVertVel_ = 0.0f;
    playerOnGround_ = false;

    // If we spawned intersecting the voxel surface, push out a few iterations.
    for (int i = 0; i < collisionIters_; ++i)
    {
        ResolveHorizontalCollisions(playerFeetPos_, up);
        bool grounded = false;
        float vv = 0.0f;
        ResolveVerticalCollisions(playerFeetPos_, up, vv, grounded);
    }

    camera_.Position = playerFeetPos_ + up * playerEyeHeight_;
}

void App::UpdatePlayerPhysics(float dt)
{
    if (dt <= 0.0f) return;

    // Safety: first frame after boot / toggling.
    if (glm::dot(playerFeetPos_, playerFeetPos_) < 1e-6f)
        InitPlayerFromCamera();

    // Planet up (radial).
    glm::vec3 up = glm::normalize(camera_.Position);
    if (glm::length(up) < 1e-6f) up = glm::vec3(0, 1, 0);

    // Clamp combined inputs (keyboard + gamepad can sum > 1)
    moveForward_ = std::max(-1.0f, std::min(1.0f, moveForward_));
    moveRight_   = std::max(-1.0f, std::min(1.0f, moveRight_));

    // Tangent-plane basis (same idea as Camera::ProcessKeyboard in FPS mode)
    glm::vec3 fwd = camera_.Front - up * glm::dot(camera_.Front, up);
    float fLen2 = glm::dot(fwd, fwd);
    if (fLen2 > 1e-6f)
        fwd /= std::sqrt(fLen2);
    else
        fwd = glm::normalize(glm::cross(up, camera_.Right));

    glm::vec3 right = glm::normalize(glm::cross(fwd, up));

    glm::vec3 wish = fwd * moveForward_ + right * moveRight_;
    float wishLen2 = glm::dot(wish, wish);
    if (wishLen2 > 1e-6f) wish /= std::sqrt(wishLen2);

    float speed = walkSpeed_;
    if (sprintHeld_) speed *= sprintMultiplier_;

    glm::vec3 horizVel = wish * speed;

    // Jump
    if (jumpRequested_ && playerOnGround_)
    {
        playerVertVel_ = jumpSpeed_;
        playerOnGround_ = false;
    }

    // Gravity (along -up)
    playerVertVel_ -= gravity_ * dt;
    playerVertVel_ = std::max(playerVertVel_, -60.0f); // terminal velocity

    // -----------------------------------------------------------------
    // 1) Horizontal move + collisions (no auto-step: we DO NOT allow the
    //    solver to push the player upward in this phase).
    // -----------------------------------------------------------------
    playerFeetPos_ += horizVel * dt;
    ResolveHorizontalCollisions(playerFeetPos_, up);

    // -----------------------------------------------------------------
    // 2) Vertical move + collisions (ground/ceiling)
    // -----------------------------------------------------------------
    playerFeetPos_ += up * (playerVertVel_ * dt);

    bool grounded = false;
    ResolveVerticalCollisions(playerFeetPos_, up, playerVertVel_, grounded);
    playerOnGround_ = grounded;

    // Drive the camera from the physics feet position.
    camera_.Position = playerFeetPos_ + up * playerEyeHeight_;
}

static inline glm::vec3 ClosestPointOnAABB(const glm::vec3& p, const glm::vec3& bmin, const glm::vec3& bmax)
{
    return glm::vec3(
        std::max(bmin.x, std::min(bmax.x, p.x)),
        std::max(bmin.y, std::min(bmax.y, p.y)),
        std::max(bmin.z, std::min(bmax.z, p.z))
    );
}

void App::ResolveHorizontalCollisions(glm::vec3& feetPos, const glm::vec3& up)
{
    const float r = playerRadius_;
    const float skin = 0.001f;

    const float usable = std::max(0.0f, playerHeight_ - 2.0f * r);
    const int samples = std::max(3, (int)std::ceil(usable / 0.50f) + 1); // sample every ~0.5 voxel

    auto solveSphere = [&](glm::vec3& center) -> bool
    {
        bool hit = false;

        glm::vec3 minP = center - glm::vec3(r);
        glm::vec3 maxP = center + glm::vec3(r);

        int minX = (int)std::floor(minP.x);
        int minY = (int)std::floor(minP.y);
        int minZ = (int)std::floor(minP.z);
        int maxX = (int)std::floor(maxP.x);
        int maxY = (int)std::floor(maxP.y);
        int maxZ = (int)std::floor(maxP.z);

        for (int z = minZ; z <= maxZ; ++z)
            for (int y = minY; y <= maxY; ++y)
                for (int x = minX; x <= maxX; ++x)
                {
                    Block b = world_.GetBlock(x, y, z);
                    if (!IsCollidableBlock(b)) continue;

                    glm::vec3 bmin((float)x, (float)y, (float)z);
                    glm::vec3 bmax = bmin + glm::vec3(1.0f);

                    glm::vec3 closest = ClosestPointOnAABB(center, bmin, bmax);
                    glm::vec3 d = center - closest;
                    float d2 = glm::dot(d, d);

                    const float rr = r * r;
                    if (d2 >= rr) continue;

                    glm::vec3 push(0.0f);

                    if (d2 > 1e-10f)
                    {
                        float dist = std::sqrt(d2);
                        glm::vec3 n = d / dist;
                        float pen = (r - dist) + skin;
                        push = n * pen;
                    }
                    else
                    {
                        // Sphere center is inside the AABB: push out through the nearest face.
                        float dx0 = center.x - bmin.x;
                        float dx1 = bmax.x - center.x;
                        float dy0 = center.y - bmin.y;
                        float dy1 = bmax.y - center.y;
                        float dz0 = center.z - bmin.z;
                        float dz1 = bmax.z - center.z;

                        float minDist = dx0;
                        glm::vec3 n(-1, 0, 0);

                        if (dx1 < minDist) { minDist = dx1; n = glm::vec3( 1, 0, 0); }
                        if (dy0 < minDist) { minDist = dy0; n = glm::vec3( 0,-1, 0); }
                        if (dy1 < minDist) { minDist = dy1; n = glm::vec3( 0, 1, 0); }
                        if (dz0 < minDist) { minDist = dz0; n = glm::vec3( 0, 0,-1); }
                        if (dz1 < minDist) { minDist = dz1; n = glm::vec3( 0, 0, 1); }

                        float pen = (minDist + r) + skin;
                        push = n * pen;
                    }

                    // NO AUTO-STEP: remove any 'up' component from horizontal resolution.
                    push -= up * glm::dot(push, up);

                    if (glm::dot(push, push) > 1e-10f)
                    {
                        feetPos += push;
                        center += push;
                        hit = true;
                    }
                }

        return hit;
    };

    for (int iter = 0; iter < collisionIters_; ++iter)
    {
        bool any = false;

        for (int si = 0; si < samples; ++si)
        {
            float t = (samples <= 1) ? 0.0f : (float)si / (float)(samples - 1);
            glm::vec3 center = feetPos + up * (r + usable * t);
            any |= solveSphere(center);
        }

        if (!any) break;
    }
}

void App::ResolveVerticalCollisions(glm::vec3& feetPos, const glm::vec3& up, float& vertVel, bool& onGround)
{
    const float r = playerRadius_;
    const float skin = 0.001f;

    const float usable = std::max(0.0f, playerHeight_ - 2.0f * r);
    const int samples = std::max(3, (int)std::ceil(usable / 0.50f) + 1);

    onGround = false;

    auto solveSphere = [&](glm::vec3& center) -> bool
    {
        bool hit = false;

        glm::vec3 minP = center - glm::vec3(r);
        glm::vec3 maxP = center + glm::vec3(r);

        int minX = (int)std::floor(minP.x);
        int minY = (int)std::floor(minP.y);
        int minZ = (int)std::floor(minP.z);
        int maxX = (int)std::floor(maxP.x);
        int maxY = (int)std::floor(maxP.y);
        int maxZ = (int)std::floor(maxP.z);

        for (int z = minZ; z <= maxZ; ++z)
            for (int y = minY; y <= maxY; ++y)
                for (int x = minX; x <= maxX; ++x)
                {
                    Block b = world_.GetBlock(x, y, z);
                    if (!IsCollidableBlock(b)) continue;

                    glm::vec3 bmin((float)x, (float)y, (float)z);
                    glm::vec3 bmax = bmin + glm::vec3(1.0f);

                    glm::vec3 closest = ClosestPointOnAABB(center, bmin, bmax);
                    glm::vec3 d = center - closest;
                    float d2 = glm::dot(d, d);

                    const float rr = r * r;
                    if (d2 >= rr) continue;

                    glm::vec3 push(0.0f);

                    if (d2 > 1e-10f)
                    {
                        float dist = std::sqrt(d2);
                        glm::vec3 n = d / dist;
                        float pen = (r - dist) + skin;
                        push = n * pen;
                    }
                    else
                    {
                        float dx0 = center.x - bmin.x;
                        float dx1 = bmax.x - center.x;
                        float dy0 = center.y - bmin.y;
                        float dy1 = bmax.y - center.y;
                        float dz0 = center.z - bmin.z;
                        float dz1 = bmax.z - center.z;

                        float minDist = dx0;
                        glm::vec3 n(-1, 0, 0);

                        if (dx1 < minDist) { minDist = dx1; n = glm::vec3( 1, 0, 0); }
                        if (dy0 < minDist) { minDist = dy0; n = glm::vec3( 0,-1, 0); }
                        if (dy1 < minDist) { minDist = dy1; n = glm::vec3( 0, 1, 0); }
                        if (dz0 < minDist) { minDist = dz0; n = glm::vec3( 0, 0,-1); }
                        if (dz1 < minDist) { minDist = dz1; n = glm::vec3( 0, 0, 1); }

                        float pen = (minDist + r) + skin;
                        push = n * pen;
                    }

                    // Only treat as ground/ceiling if the separation direction is reasonably aligned with 'up'.
                    float pushLen2 = glm::dot(push, push);
                    if (pushLen2 <= 1e-10f)
                        continue;

                    float align = std::abs(glm::dot(push / std::sqrt(pushLen2), up));
                    if (align < 0.35f)
                        continue;

                    // Vertical-only: keep only the component along up.
                    float pushUp = glm::dot(push, up);
                    glm::vec3 pushV = up * pushUp;

                    if (glm::dot(pushV, pushV) <= 1e-10f)
                        continue;

                    feetPos += pushV;
                    center += pushV;
                    hit = true;

                    if (pushUp > 0.0f)
                    {
                        // Pushed outward: we hit ground (only matters if we were falling)
                        if (vertVel < 0.0f) vertVel = 0.0f;
                        onGround = true;
                    }
                    else if (pushUp < 0.0f)
                    {
                        // Pushed inward: we hit a ceiling while jumping
                        if (vertVel > 0.0f) vertVel = 0.0f;
                    }
                }

        return hit;
    };

    for (int iter = 0; iter < collisionIters_; ++iter)
    {
        bool any = false;

        for (int si = 0; si < samples; ++si)
        {
            float t = (samples <= 1) ? 0.0f : (float)si / (float)(samples - 1);
            glm::vec3 center = feetPos + up * (r + usable * t);
            any |= solveSphere(center);
        }

        if (!any) break;
    }
}

void App::ToggleMouseCapture()
{
    mouseCap_ = !mouseCap_;

    if (mouseCap_)
    {
        // Save cursor position so we can restore it when unlocking.
        glfwGetCursorPos(window_, &savedX_, &savedY_);
        capSpot_ = false;
        firstMouse_ = true;
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursorPos(window_, savedX_, savedY_);
    }
}

void App::ProcessGamepadInput()
{
    if (!glfwJoystickPresent(gamepadId_)) return;
    if (!glfwJoystickIsGamepad(gamepadId_)) return;

    GLFWgamepadstate state{};
    if (!glfwGetGamepadState(gamepadId_, &state)) return;

    // B / Circle closes the app (edge-triggered)
    if (state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS)
    {
        if (!gpBHeld_) glfwSetWindowShouldClose(window_, true);
        gpBHeld_ = true;
    }
    else
    {
        gpBHeld_ = false;
    }

    



    // Left stick click (L3) toggles Fly/FPS movement (press once)
    if (state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] == GLFW_PRESS)
    {
        gpL3Held_ = true;
    }
    else if (gpL3Held_)
    {
        ToggleFlyMode();
        gpL3Held_ = false;
    }

    // Right stick click (R3) toggles camera look/capture (same as C)
    if (state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] == GLFW_PRESS)
    {
        gpR3Held_ = true;
    }
    else if (gpR3Held_)
    {
        ToggleMouseCapture();
        gpR3Held_ = false;
    }


    // Left stick = movement (fly mode drives the camera directly; FPS mode feeds the physics)
    const float lx  = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepadDeadzone_);
    const float ly  = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], gamepadDeadzone_);
    const float lLT = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], gamepadDeadzone_);

    // Sprint (Left Trigger)
    gpLTHeld_ = (lLT > 0.f);
    if (gpLTHeld_) sprintHeld_ = true;

    // Jump (A) in FPS mode (edge-triggered)
    if (!camera_.IsFlyMode())
    {
        if (state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS)
        {
            if (!gpAHeld_) jumpRequested_ = true;
            gpAHeld_ = true;
        }
        else
        {
            gpAHeld_ = false;
        }
    }

    const float forward = -ly; // GLFW: up is typically negative

    if (camera_.IsFlyMode())
    {
        // Fly movement stays immediate
        if (forward > 0.0f)
            camera_.ProcessKeyboard(FORWARD, deltaTime_ * forward, gpLTHeld_, 1.2f);
        else if (forward < 0.0f)
            camera_.ProcessKeyboard(BACKWARD, deltaTime_ * (-forward), gpLTHeld_, 1.1f);

        if (lx > 0.0f)
            camera_.ProcessKeyboard(RIGHT, deltaTime_ * lx);
        else if (lx < 0.0f)
            camera_.ProcessKeyboard(LEFT, deltaTime_ * (-lx));
    }
    else
    {
        // FPS mode: accumulate into movement axes for UpdatePlayerPhysics
        moveForward_ += forward;
        moveRight_ += lx;
    }

    // Right stick = mouse look (only when "captured" / enabled)
    if (mouseCap_)
    {
        const float rxRaw = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepadDeadzone_);
        const float ryRaw = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], gamepadDeadzone_);

        // "Flick to full speed" look:
        // - Fast stick movement -> instant response (no ramp)
        // - Slow stick movement -> smoother ramp (more precision)
        // We implement this by smoothing the stick input with an alpha that depends on
        // how quickly the stick is moving.
        auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        auto curveExpo = [&](float v)
            {
                float av = std::fabs(v);
                float cv = std::pow(av, gamepadLookExpo_);
                return (v < 0.0f) ? -cv : cv;
            };

        // Snap to zero when released so we don't get "drift" from smoothing.
        if (rxRaw == 0.0f && ryRaw == 0.0f)
        {
            gpRightXSmooth_ = gpRightYSmooth_ = 0.0f;
            gpRightXPrev_ = gpRightYPrev_ = 0.0f;
        }
        else
        {
            const float dtSafe = std::max(deltaTime_, 1e-6f);

            const float dx = rxRaw - gpRightXPrev_;
            const float dy = ryRaw - gpRightYPrev_;
            const float speed = std::sqrt(dx * dx + dy * dy) / dtSafe; // stick-units per second

            // Convert "time to go 0->1" into a speed threshold.
            float fastSpeed = (gamepadLookFastTime_ > 1e-6f) ? (1.0f / gamepadLookFastTime_) : 1000.0f;
            float slowSpeed = (gamepadLookSlowTime_ > 1e-6f) ? (1.0f / gamepadLookSlowTime_) : fastSpeed;
            if (slowSpeed > fastSpeed) std::swap(slowSpeed, fastSpeed);

            float t = 1.0f;
            if (fastSpeed != slowSpeed)
                t = (speed - slowSpeed) / (fastSpeed - slowSpeed);
            t = clamp01(t);

            float alpha = lerp(gamepadLookMinAlpha_, 1.0f, t);
            alpha = clamp01(alpha);

            gpRightXSmooth_ = lerp(gpRightXSmooth_, rxRaw, alpha);
            gpRightYSmooth_ = lerp(gpRightYSmooth_, ryRaw, alpha);

            gpRightXPrev_ = rxRaw;
            gpRightYPrev_ = ryRaw;
        }

        const float rx = curveExpo(gpRightXSmooth_);
        const float ry = curveExpo(gpRightYSmooth_);

        // Convert stick [-1..1] -> degrees/sec -> ProcessMouseMovement units
        const float sens = (camera_.MouseSensitivity != 0.0f) ? camera_.MouseSensitivity : 0.1f;
        const float xoffset = (rx * gamepadLookDegPerSec_ * deltaTime_) / sens;
        const float yoffset = (-ry * gamepadLookDegPerSec_ * deltaTime_) / sens;

        if (xoffset != 0.0f || yoffset != 0.0f)
            camera_.ProcessMouseMovement(xoffset, yoffset);
    }
}

void App::ProcessInput()
{
    // Reset per-frame aggregated movement input (FPS mode uses these, fly mode ignores them)
    moveForward_ = 0.0f;
    moveRight_ = 0.0f;
    sprintHeld_ = false;
    jumpRequested_ = false;

    // During the loading screen, only allow quitting (ESC / B).
    if (loading_)
    {
        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window_, true);

        ProcessGamepadInput();
        return;
    }

    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window_, true);

    // Sprint (keyboard)
    if (glfwGetKey(window_, GLFW_KEY_P) == GLFW_PRESS)
        sprintHeld_ = true;

    // --- Keyboard movement ---
    if (camera_.IsFlyMode())
    {
        // Fly mode keeps the old freecam translation
        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
            camera_.ProcessKeyboard(FORWARD, deltaTime_, sprintHeld_, 1.2f);
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
            camera_.ProcessKeyboard(BACKWARD, deltaTime_, sprintHeld_, 1.1f);
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
            camera_.ProcessKeyboard(LEFT, deltaTime_, sprintHeld_, 1.0f);
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
            camera_.ProcessKeyboard(RIGHT, deltaTime_, sprintHeld_, 1.0f);
    }
    else
    {
        // FPS mode uses voxel collision + gravity (UpdatePlayerPhysics)
        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) moveForward_ += 1.0f;
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) moveForward_ -= 1.0f;
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) moveRight_ += 1.0f;
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) moveRight_ -= 1.0f;

        // Jump (edge-triggered)
        if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            if (!spaceHeld_)
            {
                jumpRequested_ = true;
                spaceHeld_ = true;
            }
        }
        else
        {
            spaceHeld_ = false;
        }
    }

    // Toggle fly/FPS movement with F (press once)
    if (glfwGetKey(window_, GLFW_KEY_F) == GLFW_PRESS) {
        fHeld_ = true;
    }
    if (glfwGetKey(window_, GLFW_KEY_F) == GLFW_RELEASE) {
        if (fHeld_) {
            ToggleFlyMode();
            fHeld_ = false;
        }
    }

    // Toggle capture with C (press once)
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS) {
        cHeld_ = true;
    }
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_RELEASE) {
        if (cHeld_) {
            ToggleMouseCapture();
            cHeld_ = false;
        }
    }

    // Gamepad can contribute movement/jump, or drive flycam movement.
    ProcessGamepadInput();
}