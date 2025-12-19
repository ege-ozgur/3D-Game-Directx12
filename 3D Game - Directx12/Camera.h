#pragma once
#include "maths.h"

class Camera { // This is a simple camera class to hold position, rotation, view and projection matrices
public:
    Vec3 position;
    Vec3 rotation; 
    Matrix4x4 viewMatrix;
    Matrix4x4 projMatrix;

	Camera() { // default constructor sets position and rotation
        position = Vec3(0, 10, -30); 
        rotation = Vec3(0, 0, 0);
    }

	void lookAt(Vec3 from, Vec3 to, Vec3 up) { // sets the view matrix using lookAt function in our maths.h file
        viewMatrix = Matrix4x4::lookAtMatrix(from, to, up); 
    }


	void setPerspective(float fovDegrees, float aspectRatio, float nearZ, float farZ) { // sets the projection matrix using perspective projection function in our maths.h file
        float fovRadians = fovDegrees * (3.14159f / 180.0f);
        projMatrix = projMatrix.perspectiveProjection(aspectRatio, fovRadians, nearZ, farZ);
    }

	Matrix4x4 getVP() { // It returns the view-projection matrix
        return viewMatrix.multiply(projMatrix);
    }
};