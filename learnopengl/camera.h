#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <cmath>
// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 20.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    // Movement mode
    bool FlyMode = false; // false = FPS (tangent-plane), true = fly (freecam)

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;

        glm::vec3 up = glm::normalize(WorldUp);

        // Fly: W/S follows Front (including vertical).
        // FPS: W/S is projected onto the tangent plane (no flying).
        glm::vec3 fwd = glm::normalize(Front);
        if (!FlyMode)
        {
            fwd = Front - up * glm::dot(Front, up);
            float fwdLen2 = glm::dot(fwd, fwd);
            if (fwdLen2 > 1e-6f) fwd /= std::sqrt(fwdLen2);
            else fwd = glm::normalize(glm::cross(up, Right)); // fallback
        }

        if (direction == FORWARD)  Position += fwd * velocity;
        if (direction == BACKWARD) Position -= fwd * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    void SetWorldUp(const glm::vec3& up)
    {
        WorldUp = glm::normalize(up);
        updateCameraVectors(); // re-orthonormalize to remove roll drift
    }

    void SetFlyMode(bool enabled) { FlyMode = enabled; }
    void ToggleFlyMode() { FlyMode = !FlyMode; }
    bool IsFlyMode() const { return FlyMode; }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // Build a stable local frame from WorldUp (planet up).
        glm::vec3 up = glm::normalize(WorldUp);

        // Choose a reference axis that isn't parallel to up
        glm::vec3 ref = (std::abs(up.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);

        // east/right axis on the tangent plane
        glm::vec3 east = glm::normalize(glm::cross(ref, up));
        // north/forward-on-plane axis
        glm::vec3 north = glm::normalize(glm::cross(up, east));

        // Yaw rotates around 'up' in the tangent plane, pitch rotates toward up/down
        float yawRad = glm::radians(Yaw);
        float pitchRad = glm::radians(Pitch);

        glm::vec3 f =
            north * (cos(pitchRad) * cos(yawRad)) +
            east * (cos(pitchRad) * sin(yawRad)) +
            up * (sin(pitchRad));

        Front = glm::normalize(f);

        // Recompute Right/Up with NO roll: Up is locked to WorldUp direction
        Right = glm::normalize(glm::cross(Front, up));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif