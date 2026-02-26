#include "App.h"
#include <iostream>
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
    world_.planet.maxHeight = 64.0f;
    world_.planet.noiseFreq = 16.0f;
    world_.planet.octaves = 2;

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

    return true;
}

int App::Run()
{
    if (!InitWindow()) return -1;
    if (!InitGL()) return -1;
    if (!LoadAssets()) return -1;

    camera_.Position = glm::vec3(0.0f, 0.0f, world_.planet.baseRadius + 20.0f);


    glClearColor(.16f, .46f, 96.f, 1.0f);


    //world_.BuildPlanetOnce();
    while (!glfwWindowShouldClose(window_))
    {
        float current = (float)glfwGetTime();
        deltaTime_ = current - lastFrame_;
        lastFrame_ = current;
        
        // Lock camera "up" to planet radial up (prevents roll)
        glm::vec3 radialUp = glm::normalize(camera_.Position);
      
        camera_.SetWorldUp(glm::normalize(camera_.Position));
        ProcessInput(); 
        camera_.SetWorldUp(glm::normalize(camera_.Position));

        world_.UpdateStreaming(camera_.Position, camera_.Front);
        world_.TickBuildQueues(2,4);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera_.Zoom),
            (float)width_ / (float)height_, 0.1f, 2000.0f);

        glm::mat4 view = camera_.GetViewMatrix();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, blockTexArray_);

       
        voxelShader_->use();
        voxelShader_->setMat4("projection", projection);
        voxelShader_->setMat4("view", view);

        voxelShader_->setVec3("uCameraPos", camera_.Position);
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

    float xoffset  = xpos - lastX_;
    float yoffset = lastY_ - ypos;
    xoffset *= 1000.f;
    yoffset *= 1000.f;


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
    std::cout << (camera_.IsFlyMode() ? "[Camera] Fly mode ON\n" : "[Camera] FPS mode ON\n");
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

    // Left stick = WASD
    const float lx = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepadDeadzone_);
    const float ly = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], gamepadDeadzone_);

    const float forward = -ly; // GLFW: up is typically negative
    if (forward > 0.0f)      camera_.ProcessKeyboard(FORWARD,  deltaTime_ * forward);
    else if (forward < 0.0f) camera_.ProcessKeyboard(BACKWARD, deltaTime_ * (-forward));

    if (lx > 0.0f)      camera_.ProcessKeyboard(RIGHT, deltaTime_ * lx);
    else if (lx < 0.0f) camera_.ProcessKeyboard(LEFT,  deltaTime_ * (-lx));

    // Right stick = mouse look (only when "captured" / enabled)
    if (mouseCap_)
    {
        const float rx = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepadDeadzone_);
        const float ry = ApplyDeadzone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], gamepadDeadzone_);

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
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window_, true);

    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) camera_.ProcessKeyboard(FORWARD, deltaTime_);
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) camera_.ProcessKeyboard(BACKWARD, deltaTime_);
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) camera_.ProcessKeyboard(LEFT, deltaTime_);
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) camera_.ProcessKeyboard(RIGHT, deltaTime_);
    // Toggle fly/FPS movement with F (press once)
    if (glfwGetKey(window_, GLFW_KEY_F) == GLFW_PRESS) {
        fHeld_ = true;
    }
    if (glfwGetKey(window_, GLFW_KEY_F) == GLFW_RELEASE) {
        if (fHeld_) {
            camera_.ToggleFlyMode();
//            std::cout << (camera_.IsFlyMode() ? "[Camera] Fly mode ON\n" : "[Camera] FPS mode ON\n");
            fHeld_ = false;
        }
    }

    // Toggle capture with C (same logic you had)
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS) {
        cHeld_ = true;
    }
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_RELEASE) {
           if (cHeld_) {
        ToggleMouseCapture();
        cHeld_ = false;

        }
    }
ProcessGamepadInput();
}