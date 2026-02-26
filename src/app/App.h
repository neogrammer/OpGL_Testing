#pragma once
#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <learnopengl/shader_s.h>
#include <learnopengl/camera.h>
#include "src/voxel/World.h"

#include <../src/app/GpuMesh.h>

class App {
public:
    int Run();
    std::unique_ptr<Shader> oceanShader_;
    GpuMesh oceanMesh_;
    bool oceanBuilt_ = false;
    int gamepadId_=0;
    bool gpBHeld_, gpL3Held_, gpLTHeld_, gpR3Held_ = false;
    float gamepadDeadzone_ = 0.3f;
    float gamepadLookDegPerSec_ = 40.f;

    void BuildOceanMesh(float radius, int stacks = 96, int slices = 192);
private:
    static constexpr int INIT_W = 2560;
    static constexpr int INIT_H = 1600;

    // Startup loading screen
    bool loading_ = true;
    double loadingTitleT0_ = 0.0;

    // Streaming budgets: tune per machine
    int loadGenPerFrame_ = 2;
    int loadMeshPerFrame_ = 4;
    int playGenPerFrame_ = 1;
    int playMeshPerFrame_ = 3;

    GLFWwindow* window_ = nullptr;
    int width_ = INIT_W;
    int height_ = INIT_H;

    Camera camera_{ glm::vec3(0.0f, 0.0f, 3.0f) };

    float lastX_ = INIT_W / 2.0f;
    float lastY_ = INIT_H / 2.0f;
    bool firstMouse_ = true;

    float deltaTime_ = 0.0f;
    float lastFrame_ = 0.0f;

    bool mouseCap_ = false;
    bool capSpot_ = false;
    double savedX_ = 0.0;
    double savedY_ = 0.0;
    bool cHeld_ = false;
    bool fHeld_ = false;

    // --- Player scale ---
    // Camera height above the terrain surface in *voxel/world units*.
    // 0.75 = 'top half of the first voxel' (Minecraft eye height is ~1.62)
    float playerEyeHeight_ = 2.5f;
    bool  spawnAboveSea_ = true;

    std::unique_ptr<Shader> voxelShader_;
    World world_;

    GLuint blockTexArray_ = 0;
    int texW_ = 0, texH_ = 0;

    bool InitWindow();
    bool InitGL();
    bool LoadAssets();
    void ProcessInput();
    float ApplyDeadzone(float v, float dz);
        void ToggleMouseCapture();
        void ToggleFlyMode();
        void SnapCameraToSurface(bool keepAboveSea);
    void ProcessGamepadInput();
    void OnResize(int w, int h);
    void OnMouse(double x, double y);
    void OnScroll(double xoff, double yoff);
};