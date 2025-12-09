#pragma once
#include "Maths.h"
#include "Core.h"
#include "Window.h"
#include "Collision.h" 
#include <vector>
#include <cmath>

class Player {
public:
    Vec3 position;
    Vec3 rotation;
    Vec3 velocity;

    int currentAmmo = 30;
    int maxClipAmmo = 30;
    int totalAmmo = 120;

    float fireRate = 0.15f;
    float fireTimer = 0.0f;

    bool isFiring = false;
    bool isReloading = false;
    bool isMoving = false;

    float health = 100.0f;
    float speed = 15.0f;
    float mouseSensitivity = 0.002f;
    float eyeHeight = 1.7f;
    Vec3 colliderSize = Vec3(2.0f, 4.0f, 2.0f);

    void init(Vec3 startPos) {
        position = startPos;
        rotation = Vec3(0, 0, 0);
        velocity = Vec3(0, 0, 0);
    }

    void startReload() {
        if (isReloading || totalAmmo <= 0 || currentAmmo >= maxClipAmmo) {
            return;
        }          
        isReloading = true;
    }

    void completeReload() {
        isReloading = false;

        int needed = maxClipAmmo - currentAmmo;
        if (totalAmmo >= needed) {
            totalAmmo -= needed;
            currentAmmo = maxClipAmmo;
        }
        else {
            currentAmmo += totalAmmo;
            totalAmmo = 0;
        }
    }

    void update(float dt, Window* win, const std::vector<AABB>& obstacles) {
        if (fireTimer > 0.0f) fireTimer -= dt;

        isFiring = false;

        if (currentAmmo <= 0 && totalAmmo > 0 && !isReloading) {
            startReload();
        }

        if ((GetAsyncKeyState('R') & 0x8000) || win->keys['R']) {
            startReload();
        }

        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && !isReloading && currentAmmo > 0 && fireTimer <= 0.0f) {
            isFiring = true;
            currentAmmo--;
            fireTimer = fireRate;
        }

        POINT cursorPos;
        GetCursorPos(&cursorPos);
        RECT rect;
        GetWindowRect(win->hwnd, &rect);
        int centerX = (rect.left + rect.right) / 2;
        int centerY = (rect.top + rect.bottom) / 2;

        if (GetForegroundWindow() == win->hwnd) {
            float deltaX = (float)(cursorPos.x - centerX);
            float deltaY = (float)(cursorPos.y - centerY);

            rotation.y += deltaX * mouseSensitivity;
            rotation.x += deltaY * mouseSensitivity;

            if (rotation.x > 1.5f) {
                rotation.x = 1.5f;
            }
                
            if (rotation.x < -1.5f) {
                rotation.x = -1.5f;
            }
                
            SetCursorPos(centerX, centerY);
        }

        Vec3 forwardFlat;
        forwardFlat.x = sinf(rotation.y);
        forwardFlat.y = 0;
        forwardFlat.z = cosf(rotation.y);
        forwardFlat.normalize();

        Vec3 rightFlat = forwardFlat.Cross(Vec3(0, 1, 0)).normalize();

        Vec3 moveDir(0, 0, 0);

        if (win->keys['W']) moveDir += forwardFlat;
        if (win->keys['S']) moveDir -= forwardFlat;
        if (win->keys['D']) moveDir -= rightFlat;
        if (win->keys['A']) moveDir += rightFlat;

        if (moveDir.x != 0 || moveDir.z != 0) {
            moveDir = moveDir.normalize();
            isMoving = true;
        }
        else {
            isMoving = false;
        }

        Vec3 desiredMove = moveDir * speed * dt;

        Vec3 nextPosX = position;
        nextPosX.x += desiredMove.x;

        AABB playerBoxX;
        playerBoxX.position = nextPosX;
        playerBoxX.position.y += (colliderSize.y * 0.5f);
        playerBoxX.size = colliderSize;

        bool hitX = false;
        for (const auto& box : obstacles) {
            if (AABB::check(playerBoxX, box)) {
                hitX = true;
                break;
            }
        }
        if (!hitX) position.x += desiredMove.x;

        Vec3 nextPosZ = position;
        nextPosZ.z += desiredMove.z;

        AABB playerBoxZ;
        playerBoxZ.position = nextPosZ;
        playerBoxZ.position.y += (colliderSize.y * 0.5f);
        playerBoxZ.size = colliderSize;

        bool hitZ = false;
        for (const auto& box : obstacles) {
            if (AABB::check(playerBoxZ, box)) {
                hitZ = true;
                break;
            }
        }
        if (!hitZ) position.z += desiredMove.z;
    }

    Matrix getViewMatrix() {
        Vec3 eyePos = position;
        eyePos.y += eyeHeight;

        Vec3 lookDir;
        lookDir.x = sinf(rotation.y) * cosf(rotation.x);
        lookDir.y = -sinf(rotation.x);
        lookDir.z = cosf(rotation.y) * cosf(rotation.x);
        lookDir.normalize();

        Vec3 target = eyePos + lookDir;

        return Matrix::lookAtMatrix(eyePos, target, Vec3(0, 1, 0));
    }

    Vec3 getCameraPos() const
    {
        Vec3 p = position;
        p.y += eyeHeight;
        return p;
    }
};