#include <learnopengl/shader_s.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <iostream>
#include <learnopengl/camera.h>
#include "cube_renderer.h"
#include "src/voxel/World.h"

#include <stb_image.h>
#include <src/utility/TextureUtils.h>
#include <chrono>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);


// settings
const unsigned int SCR_WIDTH = 2560;
const unsigned int SCR_HEIGHT = 1600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetWindowAspectRatio(window, 2560, 1600);
    glfwSetWindowPos(window, 600, 300);
        
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
   // -----------------------------
    glEnable(0x0B71);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
 


    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // draw...
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    Shader voxelShader("voxel.vs", "voxel.fs");
    World world;
    world.planet.baseRadius = 64.0f;
    world.planet.maxHeight = 12.0f;
    world.planet.noiseFreq = 3.0f;
    world.planet.octaves = 5;


    int texW = 0, texH = 0;
    GLuint blockTexArray = util::LoadTexture2DArray({
        "assets/textures/voxel_cube_grass.png", // layer 0
        "assets/textures/voxel_cube_dirt.png",  // layer 1
        "assets/textures/voxel_cube_stone_2.png", // layer 2
        "assets/textures/voxel_cube_sand.png",  // layer 3
        "assets/textures/voxel_cube_snow.png",  // layer 4
        "assets/textures/voxel_cube_water.png", // layer 5
        }, texW, texH);

    voxelShader.use();
    voxelShader.setInt("texArray", 0);

   // world.BuildPlanetOnce();
    camera.Position = glm::vec3(0.0f, 0.0f, world.planet.baseRadius + 20.0f);
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        world.UpdateStreaming(camera.Position);
        std:
        world.TickBuildQueues(2, 1);

        glClearColor(.1f, 0.38f, 0.33f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        voxelShader.use();
        voxelShader.setMat4("projection", projection);
        voxelShader.setMat4("view", view);
        voxelShader.setInt("texArray", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, blockTexArray);
        
       

        
        // draw opaque pass
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        world.DrawOpaque();

        // draw water pass
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        world.DrawWater();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}
static bool mouseCap = false;
static bool capSpot = false;
static double s_xpos = 0;
static double s_ypos = 0;

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{


    static bool keyHeld = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        keyHeld = true;
       
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE)
    {

        if (keyHeld == true)
        {
            mouseCap = !mouseCap;

            if (mouseCap)
            {
                capSpot = true;
               // glfwGetCursorPos(window, &xpos, &ypos);//, (double)SCR_WIDTH / 2.0, (double)SCR_HEIGHT / 2.0);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else
            {

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                bool tmp = mouseCap;
                mouseCap = false;
                int w, h;

                glfwGetWindowSize(window, &w, &h);
                auto xposIn = w / 2.0;
                auto yposIn = h / 2.0;
                glfwSetCursorPos(window, s_xpos, s_ypos);
                mouseCap = tmp;

               // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            keyHeld = false;
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{


    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
   
    if (mouseCap && !capSpot)
    {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }

    if (capSpot)
    {
        capSpot = false;
        s_xpos = xposIn;
        s_ypos = yposIn;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}