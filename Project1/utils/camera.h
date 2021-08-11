#pragma once
#include "utils.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    RAISE,
    DROP
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  5.0f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  60.0f;
const float ASPECT      =  1.7f;
const float NEARZ       =  0.1f;
const float FARZ        =  100.0f;


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

    float Aspect;
    float nearZ, farZ;
    glm::mat4 viewMat, projMat;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), Aspect(ASPECT), nearZ(NEARZ), farZ(FARZ)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
        updateViewProjMat();
    }
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, float focal_length, float aspect_ratio, float nearz, float farz, float speed = SPEED, float yaw = YAW, float pitch = PITCH)
    : MouseSensitivity(SENSITIVITY) {
        Position = position;
        WorldUp = up;
        Front = target - position;
        Yaw = yaw;
        Pitch = pitch;
        Zoom = 60.0f;
        Aspect = aspect_ratio;
        nearZ = nearz;
        farZ = farz;
        MovementSpeed = speed;
        updateCameraVectors();
        updateViewProjMat();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), Aspect(ASPECT), nearZ(NEARZ), farZ(FARZ)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
        updateViewProjMat();
    }
    ~Camera() {};

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return viewMat;
    }
    glm::mat4 GetProjMatrix()
    {
        return projMat;
    }
    glm::mat4 GetViewProjMatrix()
    {
        return projMat * viewMat;
    }
    void setAspect(float aspect) {
        Aspect = aspect;
        updateViewProjMat();
    }
    void setPerspective(float aspect, float nearz, float farz){
        Aspect = aspect; nearZ = nearz; farZ = farz;
        updateViewProjMat();
    }
    void update() {
        updateCameraVectors();
        updateViewProjMat();
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == RAISE)
            Position += WorldUp * velocity;
        if (direction == DROP)
            Position -= WorldUp * velocity;
        updateViewProjMat();
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
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
        updateViewProjMat();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 120.0f)
            Zoom = 120.0f;
        updateViewProjMat();
    }
    void renderGui() {
        ImGui::DragFloat("Zoom", &Zoom, 0.001f, 0.0f, 180.0f);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        ImGui::DragFloat("Near Z", &nearZ, 0.0001f, 0.0f, 1000.0f);
        ImGui::SameLine();
        ImGui::DragFloat("Far Z", &farZ, 0.1f, 0.0f, 100000.0f);
        ImGui::DragFloat("Yaw", &Yaw, 0.01f, -360.0f, 360.0f);
        ImGui::SameLine();
        ImGui::DragFloat("Pitch", &Pitch, 0.01f, -180.0f, 180.0f);
        ImGui::PopItemWidth();
        ImGui::DragFloat3("Position", (float*)&Position[0], 0.01f, -10000.0f, 10000.0f);
        ImGui::DragFloat("MovementSpeed", &MovementSpeed, 0.01f, 0.0f, 1000.0f);
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
    void updateViewProjMat(){
        viewMat = glm::lookAt(Position, Position + Front, Up);
        projMat = glm::perspective(glm::radians(Zoom), Aspect, nearZ, farZ);
    }
};