#pragma once
#include "maths.h"

class Camera {
public:
    Vec3 position;
    Vec3 rotation; 
    Matrix4x4 viewMatrix;
    Matrix4x4 projMatrix;

    Camera() {
        position = Vec3(0, 10, -30); 
        rotation = Vec3(0, 0, 0);
    }

    void lookAt(Vec3 from, Vec3 to, Vec3 up) {
        viewMatrix = Matrix4x4::lookAtMatrix(from, to, up); 
    }


    void setPerspective(float fovDegrees, float aspectRatio, float nearZ, float farZ) {
        float fovRadians = fovDegrees * (3.14159f / 180.0f);
        projMatrix = projMatrix.perspectiveProjection(aspectRatio, fovRadians, nearZ, farZ);
    }

    Matrix4x4 getVP() {
        return viewMatrix.multiply(projMatrix);
    }
};