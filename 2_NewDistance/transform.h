#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "common.h"

struct CameraManager
{
private:
    const vec3 INIT_POS_TERRAIN =  vec3(3.9, 3.6, 0.6);
    const vec3 INIT_LOOK_TERRAIN  = vec3(3.3, 2.9, 0.33);
    const vec3 INIT_POS_MESH  = vec3(3.4, 3.4, 2.4);
    const vec3 INIT_LOOK_MESH =  vec3(2.8, 2.8, 2.0);

public:
    vec3 pos;
    vec3 look;
    vec3 up;
    vec3 right;
    vec3 direction;
    float fov;
    int render_width, render_height;

    void PrintStatus()
    {
        cout << "-- CAMERA STATUS --" << endl;
        cout << "  Position: " << glm::to_string(pos) << endl;
        cout << "  Look    : " << glm::to_string(look) << endl;
        cout << "  Up      : " << glm::to_string(up) << endl;
        cout << "  Right   : " << glm::to_string(right) << endl ;
        cout << "-- END --" << endl;
    }

    void Init(uint mode)
    {
        if (mode == TERRAIN) {
            pos = INIT_POS_TERRAIN;
            look = INIT_LOOK_TERRAIN;
        } else if (mode == MESH) {
            pos = INIT_POS_MESH;
            look = INIT_LOOK_MESH;
        }
        fov = 75.0;
        up = vec3(0.0f, 0.0f, 1.0f);
        direction = glm::normalize(look - pos);
        right = glm::normalize(glm::cross(direction, up));
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
        float fovy = 45.0;
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
        block_.V = glm::lookAt(cam.pos, cam.look, cam.up);
        block_.cam_pos = cam.pos;
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

    void Rotate(float angle, vec3 axis)
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

        block_.cam_pos = cam.pos;
        block_.fovy = cam.fov;
        block_.V = glm::lookAt(cam.pos, cam.look, cam.up);
        block_.P = glm::perspective(glm::radians( block_.fovy),
                                   cam.render_width/(float)cam.render_height,
                                   near_, far_);
        updateMV();
    }

};

#endif
