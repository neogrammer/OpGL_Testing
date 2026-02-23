#include "App.h"
#include <iostream>
#include <src/utility/TextureUtils.h>

bool App::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    glfwSetWindowPos(window_, 600, 300);

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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    return true;
}

bool App::LoadAssets()
{
    voxelShader_ = std::make_unique<Shader>("voxel.vs", "voxel.fs");

    world_.planet.baseRadius = 4096.0f;
    world_.planet.maxHeight = 128.0f;
    world_.planet.noiseFreq = 16.0f;
    world_.planet.octaves = 16;

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


    

    //world_.BuildPlanetOnce();
    while (!glfwWindowShouldClose(window_))
    {
        float current = (float)glfwGetTime();
        deltaTime_ = current - lastFrame_;
        lastFrame_ = current;

        ProcessInput();

        world_.UpdateStreaming(camera_.Position, camera_.Front);
        world_.TickBuildQueues(2, 5);

        glClearColor(.1f, 0.38f, 0.33f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera_.Zoom),
            (float)width_ / (float)height_, 0.1f, 2000.0f);

        glm::mat4 view = camera_.GetViewMatrix();

       
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, blockTexArray_);

        voxelShader_->use();
        voxelShader_->setMat4("projection", projection);
        voxelShader_->setMat4("view", view);


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
        voxelShader_->setBool("uWaterPass", true);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // see water from below
        glDisable(GL_CULL_FACE);

        // correct transparency
        glDepthMask(GL_FALSE);

        // IMPORTANT: draw water back-to-front (see next patch)
        world_.DrawWaterSorted(camera_.Position);

        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
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

void App::ProcessInput()
{
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window_, true);

    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) camera_.ProcessKeyboard(FORWARD, deltaTime_);
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) camera_.ProcessKeyboard(BACKWARD, deltaTime_);
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) camera_.ProcessKeyboard(LEFT, deltaTime_);
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) camera_.ProcessKeyboard(RIGHT, deltaTime_);

    // Toggle capture with C (same logic you had)
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS) {
        cHeld_ = true;
    }
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_RELEASE) {
        if (cHeld_) {
            mouseCap_ = !mouseCap_;
            if (mouseCap_) {
                capSpot_ = true;
                firstMouse_ = true;
                glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glfwSetCursorPos(window_, savedX_, savedY_);
            }
            cHeld_ = false;
        }
    }
}