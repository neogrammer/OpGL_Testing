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

    // --- Gamepad right-stick look feel (tweakable) ---
// If you flick the stick quickly, we respond instantly.
// If you move the stick slowly, we "ramp" toward the target more gently.
    float gamepadLookFastTime_ = 0.07f; // seconds to go 0->100% to be treated as a flick
    float gamepadLookSlowTime_ = 0.25f; // seconds to go 0->100% to be treated as a slow push
    float gamepadLookMinAlpha_ = 0.12f; // 0..1 smoothing at very slow pushes (lower = smoother)
    float gamepadLookExpo_ = 1.6f;  // >1 = finer near center, still reaches full speed at edge

    // internal right-stick smoothing state
    float gpRightXPrev_ = 0.0f;
    float gpRightYPrev_ = 0.0f;
    float gpRightXSmooth_ = 0.0f;
    float gpRightYSmooth_ = 0.0f;

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

    // --- Player physics (FPS mode) ---
    // We simulate a simple capsule (approximated with multiple spheres) oriented to the planet 'up'.
    glm::vec3 playerFeetPos_{ 0.0f };   // world-space feet (bottom of collider)
    float     playerVertVel_ = 0.0f;   // velocity along 'up' (jump / gravity)
    bool      playerOnGround_ = false;

    // Per-frame movement input (aggregated from keyboard + gamepad)
    float moveForward_ = 0.0f; // +1 forward, -1 back
    float moveRight_   = 0.0f; // +1 right,   -1 left
    bool  sprintHeld_  = false;
    bool  jumpRequested_ = false; // edge-triggered
    bool  spaceHeld_   = false;  // keyboard edge tracking
    bool  gpAHeld_     = false;  // gamepad A edge tracking

    // Collider / tuning (voxel units)
    float playerRadius_ = 0.35f; // ~Minecraft half-width
    float playerHeight_ = 3.0f;  // feet-to-head
    float gravity_      = 28.0f; // units/s^2
    float jumpSpeed_    = 9.0f;  // units/s
    float walkSpeed_    = 4.0f;  // units/s
    float sprintMultiplier_ = 1.6f;
    int   collisionIters_   = 4; // solver iterations

    void InitPlayerFromCamera();
    void UpdatePlayerPhysics(float dt);
    void ResolveHorizontalCollisions(glm::vec3& feetPos, const glm::vec3& up);
    void ResolveVerticalCollisions(glm::vec3& feetPos, const glm::vec3& up, float& vertVel, bool& onGround);

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