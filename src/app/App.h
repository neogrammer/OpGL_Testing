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

    void BuildOceanMesh(float radius, int stacks = 96, int slices = 192);
private:
    static constexpr int INIT_W = 2560;
    static constexpr int INIT_H = 1600;

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

    std::unique_ptr<Shader> voxelShader_;
    World world_;

    GLuint blockTexArray_ = 0;
    int texW_ = 0, texH_ = 0;

    bool InitWindow();
    bool InitGL();
    bool LoadAssets();
    void ProcessInput();

    void OnResize(int w, int h);
    void OnMouse(double x, double y);
    void OnScroll(double xoff, double yoff);
};