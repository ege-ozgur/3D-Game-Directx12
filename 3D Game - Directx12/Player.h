#pragma once
#include "Maths.h"
#include "Core.h"
#include "Window.h"
#include "Collision.h"
#include "AnimatedMesh.h"
#include "Enemy.h"
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
    float speed = 10.0f;
    float mouseSensitivity = 0.002f;
    float eyeHeight = 1.7f;
    Vec3 colliderSize = Vec3(0.6f, 3.5f, 0.6f);

    void init(Vec3 startPos) {
        position = startPos;
        rotation = Vec3(0, 0, 0);
        velocity = Vec3(0, 0, 0);
    }

    void startReload() {
        if (isReloading || totalAmmo <= 0 || currentAmmo >= maxClipAmmo) return;
        isReloading = true;
    }

    Vec3 getForward() const {
        Vec3 f;
        f.x = sinf(rotation.y) * cosf(rotation.x);
        f.y = -sinf(rotation.x);
        f.z = cosf(rotation.y) * cosf(rotation.x);
        return f.normalize();
    }

    Vec3 getCrosshairTarget(const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies, float maxDist = 1000.0f) {
        Vec3 camPos = getCameraPos();
        Vec3 forward = getForward();
        Vec3 targetPoint = camPos + (forward * maxDist);
        float closestDist = maxDist;

        Ray ray(camPos, forward);

        for (const auto& wall : walls) {
            float t = 0.0f;
            if (wall.rayAABB(ray, t)) {
                if (t < closestDist) {
                    closestDist = t;
                    targetPoint = ray.at(t);
                }
            }
        }

        for (auto enemy : enemies) {
            if (enemy->isDead) continue;
            float t = 0.0f;
            if (enemy->collider.rayAABB(ray, t)) {
                if (t < closestDist) {
                    closestDist = t;
                    targetPoint = ray.at(t);
                }
            }
        }
        return targetPoint;
    }

    Enemy* checkShooting(const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies, float maxDist = 1000.0f) {
        Vec3 camPos = getCameraPos();
        Vec3 forward = getForward();
        Ray ray(camPos, forward);

        float closestWallDist = maxDist;
        for (const auto& wall : walls) {
            float t = 0.0f;
            if (wall.rayAABB(ray, t)) {
                if (t < closestWallDist) closestWallDist = t;
            }
        }

        Enemy* hitEnemy = nullptr;
        float closestEnemyDist = closestWallDist;

        for (auto enemy : enemies) {
            if (enemy->isDead) continue;
            float t = 0.0f;
            if (enemy->collider.rayAABB(ray, t)) {
                if (t < closestEnemyDist) {
                    closestEnemyDist = t;
                    hitEnemy = enemy;
                }
            }
        }
        return hitEnemy;
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

    AABB getAABB(Vec3 pos) {
        Vec3 halfSize = colliderSize * 0.5f;
        Vec3 min = Vec3(pos.x - halfSize.x, pos.y, pos.z - halfSize.z);
        Vec3 max = Vec3(pos.x + halfSize.x, pos.y + colliderSize.y, pos.z + halfSize.z);
        return AABB(min, max);
    }

    void update(float dt, Window* win, const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies) {
        if (fireTimer > 0.0f) fireTimer -= dt;
        isFiring = false;

        if (currentAmmo <= 0 && totalAmmo > 0 && !isReloading) startReload();
        if ((GetAsyncKeyState('R') & 0x8000) || win->keys['R']) startReload();

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
            if (rotation.x > 1.5f) rotation.x = 1.5f;
            if (rotation.x < -1.5f) rotation.x = -1.5f;
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
        AABB playerBoxX = getAABB(nextPosX);
        bool hitX = false;

        for (const auto& box : walls) {
            if (AABB::check(playerBoxX, box)) { hitX = true; break; }
        }
        if (!hitX) {
            for (auto enemy : enemies) {
                if (!enemy->isDead && AABB::check(playerBoxX, enemy->collider)) { hitX = true; break; }
            }
        }
        if (!hitX) position.x += desiredMove.x;

        Vec3 nextPosZ = position;
        nextPosZ.z += desiredMove.z;
        AABB playerBoxZ = getAABB(nextPosZ);
        bool hitZ = false;

        for (const auto& box : walls) {
            if (AABB::check(playerBoxZ, box)) { hitZ = true; break; }
        }
        if (!hitZ) {
            for (auto enemy : enemies) {
                if (!enemy->isDead && AABB::check(playerBoxZ, enemy->collider)) { hitZ = true; break; }
            }
        }
        if (!hitZ) position.z += desiredMove.z;
    }

    Vec3 getCameraPos() const {
        Vec3 p = position;
        p.y += eyeHeight;
        return p;
    }

    Matrix getViewMatrix() {
        Vec3 eyePos = position;
        eyePos.y += eyeHeight;
        Vec3 target = eyePos + getForward();
        return Matrix::lookAtMatrix(eyePos, target, Vec3(0, 1, 0));
    }

    Matrix getRotationMatrix() const {
        Matrix rx, ry;
        rx.rotationX(rotation.x);
        ry.rotAroundY(rotation.y);
        return rx * ry;
    }
};