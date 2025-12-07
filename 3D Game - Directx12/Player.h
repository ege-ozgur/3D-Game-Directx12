#pragma once
#include "Maths.h"
#include "Core.h"
#include "Window.h"
#include "Collision.h" 
#include <vector>

class Player {
public:
    Vec3 position;
    Vec3 rotation;
    Vec3 velocity;
	float health = 100.0f;

    float speed = 5.0f;
    float mouseSensitivity = 0.002f;

    float camDistance = 8.0f;
    float camHeight = 5.0f;

    Vec3 colliderSize = Vec3(1.0f, 2.0f, 1.0f); 

    void init(Vec3 startPos) {
        position = startPos;
        rotation = Vec3(0, 0, 0);
        velocity = Vec3(0, 0, 0);
    }

    void update(float dt, Window* win, const std::vector<AABB>& obstacles) {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        RECT rect;
        GetWindowRect(win->hwnd, &rect);
        int centerX = (rect.left + rect.right) / 2;
        int centerY = (rect.top + rect.bottom) / 2;
        float deltaX = (float)(cursorPos.x - centerX);
        float deltaY = (float)(cursorPos.y - centerY);
        rotation.y += deltaX * mouseSensitivity;
        rotation.x -= deltaY * mouseSensitivity;
        SetCursorPos(centerX, centerY);
        if (rotation.x > 1.5f) 
            rotation.x = 1.5f;
        if (rotation.x < -1.5f) 
            rotation.x = -1.5f;

        Vec3 forward;
        forward.x = sinf(rotation.y);
        forward.y = 0;
        forward.z = cosf(rotation.y);
        forward = forward.normalize();
        Vec3 right = forward.Cross(Vec3(0, 1, 0)).normalize();
        Vec3 moveDir(0, 0, 0);

        if (win->keys['W']) 
            moveDir += forward;
        if (win->keys['S']) 
            moveDir -= forward;
        if (win->keys['D']) 
            moveDir -= right;
        if (win->keys['A']) 
            moveDir += right;
        if (moveDir.x != 0 || moveDir.z != 0) 
            moveDir = moveDir.normalize();

        Vec3 desiredMove = moveDir * speed * dt;

        Vec3 nextPosX = position;
        nextPosX.x += desiredMove.x;

        AABB playerBoxX;
        playerBoxX.position = nextPosX;
        playerBoxX.position.y += 1.0f; 
        playerBoxX.size = colliderSize;

        bool hitX = false;
        for (const auto& box : obstacles) {
            if (AABB::check(playerBoxX, box)) {
                hitX = true;
                break;
            }
        }
        if (!hitX) {
            position.x += desiredMove.x;
        }

        Vec3 nextPosZ = position;
        nextPosZ.z += desiredMove.z;

        AABB playerBoxZ;
        playerBoxZ.position = nextPosZ;
        playerBoxZ.position.y += 1.0f;
        playerBoxZ.size = colliderSize;

        bool hitZ = false;
        for (const auto& box : obstacles) {
            if (AABB::check(playerBoxZ, box)) {
                hitZ = true;
                break;
            }
        }
        if (!hitZ) {
            position.z += desiredMove.z;
        }
    }

    Matrix getViewMatrix() {
        Vec3 flatBackDir;
        flatBackDir.x = -sinf(rotation.y);
        flatBackDir.y = 0;
        flatBackDir.z = -cosf(rotation.y);
        Vec3 eyePos = position + (flatBackDir * camDistance);
        eyePos.y += camHeight;
        Vec3 targetPos = position;
        targetPos.y += 1.5f;
        return Matrix::lookAtMatrix(eyePos, targetPos, Vec3(0, 1, 0));
    }
};