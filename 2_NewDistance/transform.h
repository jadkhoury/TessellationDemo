#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "common.h"

#define NEWCAM

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

struct CameraManager
{
private:
    const vec3 POS_TERRAIN    = vec3(0.3695212, -0.3568606, 0.0218675);
    const float YAW_TERRAIN   = -213.0f;
    const float PITCH_TERRAIN =  -24.0f;

    const vec3 POS_MESH    = vec3(0.073993, -1.757224, 0.711335);
    const float YAW_MESH   = -270.0;
    const float PITCH_MESH =  -18.0;

    const float LOOK_SENSITIVITY   =  100.0f;
    const float MOVE_SENSITIVITY   =  2.f;
    const float SCROLL_SENSITIVITY =  0.1f;

    void updateCameraVectors()
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.z = sin(glm::radians(Pitch));
        Direction = glm::normalize(front);

        // Also re-calculate the Right and Up vector
        // Normalize the vectors, because their length gets closer to 0 the more
        // you look up or down which results in slower movement.
        Right = glm::normalize(glm::cross(Direction, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Direction));
    }

public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Direction;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Euler Angles
    float Yaw;
    float Pitch;
    // Camera options
    float look_sensitivity;
    float move_sensitivity;
    float scroll_sensitivity;

    float fov;
    int render_width, render_height;

    void PrintStatus()
    {
        cout << "-- CAMERA STATUS --" << endl;
        cout << "  Position: " << glm::to_string(Position) << endl;
        cout << "  Front   : " << glm::to_string(Direction) << endl;
        cout << "  Up      : " << glm::to_string(Up) << endl;
        cout << "  Right   : " << glm::to_string(Right) << endl ;
        cout << "  WorldUp : " << glm::to_string(WorldUp) << endl ;
        cout << "  Yaw   : " << Yaw << endl ;
        cout << "  Pitch : " << Pitch << endl ;
        cout << "-- END --" << endl;
    }

    void Init(uint mode)
    {
        if (mode == TERRAIN) {
            Position = POS_TERRAIN;
            Yaw = YAW_TERRAIN;
            Pitch = PITCH_TERRAIN;
        } else if (mode == MESH) {
            Position = POS_MESH;
            Yaw = YAW_MESH;
            Pitch = PITCH_MESH;
        }

        fov = 55.0;
        WorldUp = vec3(0.0f, 0.0f, 1.0f);
        Direction = vec3(0.0f, 1.0f, 0.0f);
        look_sensitivity = LOOK_SENSITIVITY;
        move_sensitivity = MOVE_SENSITIVITY;
        scroll_sensitivity = SCROLL_SENSITIVITY;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Direction, Up);
    }

    // Processes input received from a mouse input system.
    // Expects the offset value in both the x and y direction.
    void ProcessMouseLeft(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        Yaw   += xoffset * look_sensitivity;
        Pitch += yoffset * look_sensitivity;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
            Pitch = utility::clamp(Pitch, -89.0f, 89.0f);

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Processes input received from a mouse input system.
    // Expects the offset value in both the x and y direction.
    void ProcessMouseRight(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        Position += yoffset * move_sensitivity * WorldUp;
        Position -= xoffset * move_sensitivity * Right;

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Processes input received from a mouse scroll-wheel event.
    // Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Position += Direction * yoffset * scroll_sensitivity;
    }
};

class TransformsManager
{
private:
    struct TransformBlock
    {
        mat4 M     = mat4(1.0);
        mat4 V     = mat4(1.0);
        mat4 P     = mat4(1.0);
        mat4 MVP   = mat4(1.0);
        mat4 MV    = mat4(1.0);
        mat4 invMV = mat4(1.0);
        vec4 frustum_planes[6];

        vec3 cam_pos = vec3(1.0);
        float fovy = 55.0;
    } block_;

    GLuint bo_;
    bool modified_ = true;
    float near_, far_;


    void updateMV()
    {
        block_.MV = block_.V * block_.M;
        block_.MVP = block_.P * block_.MV;
        block_.invMV = glm::transpose(glm::inverse(block_.MV));
        updateFrustum();
        modified_ = true;
    }

    void updateFrustum()
    {
        mat4& MVP = block_.MVP;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 2; ++j) {
                block_.frustum_planes[i*2+j].x = MVP[0][3] + (j == 0 ? MVP[0][i] : -MVP[0][i]);
                block_.frustum_planes[i*2+j].y = MVP[1][3] + (j == 0 ? MVP[1][i] : -MVP[1][i]);
                block_.frustum_planes[i*2+j].z = MVP[2][3] + (j == 0 ? MVP[2][i] : -MVP[2][i]);
                block_.frustum_planes[i*2+j].w = MVP[3][3] + (j == 0 ? MVP[3][i] : -MVP[3][i]);
                vec3 tmp = vec3(block_.frustum_planes[i*2+j]);
                block_.frustum_planes[i*2+j] *= glm::length(tmp);
            }
    }

public:
    GLuint GetBo() {
        return bo_;
    }

    void Upload()
    {
        if(modified_) {
            glNamedBufferSubData(bo_, 0, sizeof(TransformBlock), &block_);
            modified_ = false;
        }
    }

    void UpdateForNewView(CameraManager& cam)
    {
        block_.V = cam.GetViewMatrix();
        block_.cam_pos = cam.Position;
        updateMV();
    }

    void UpdateForNewFOV(CameraManager& cam)
    {
        block_.fovy = cam.fov;
        block_.P = glm::perspective(glm::radians(block_.fovy),
                                   cam.render_width/(float)cam.render_height,
                                   near_, far_);
        updateMV();
    }

    void UpdateForNewSize(CameraManager& cam)
    {
        block_.P = glm::perspective(glm::radians(block_.fovy),
                                   cam.render_width/(float)cam.render_height,
                                   near_, far_);
        updateMV();
    }

    void RotateModel(float angle, vec3 axis)
    {
        block_.M = glm::rotate(block_.M, angle, axis);
        updateMV();
    }

    void Init(CameraManager& cam)
    {
        utility::EmptyBuffer(&bo_);
        glCreateBuffers(1, &bo_);
        glNamedBufferStorage(bo_, sizeof(TransformBlock), NULL, GL_DYNAMIC_STORAGE_BIT);

        near_ = 0.01f;
        far_ = 1024.0f;

        block_.cam_pos = cam.Position;
        block_.fovy = cam.fov;
        block_.V = cam.GetViewMatrix();
        block_.P = glm::perspective(glm::radians( block_.fovy),
                                   cam.render_width/(float)cam.render_height,
                                   near_, far_);
        updateMV();
    }

    void CleanUp()
    {
        delete &block_;
        utility::EmptyBuffer(&bo_);
    }

};

#endif
