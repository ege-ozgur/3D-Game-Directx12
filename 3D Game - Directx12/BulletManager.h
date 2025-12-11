#pragma once
#include "Sphere.h" 
#include "Maths.h"
#include "Collision.h"
#include "EnemyManager.h"
#include <vector>

struct Bullet {
    Vec3 position;
    Vec3 direction;
    float speed = 50.0f;
    float lifeTime = 3.0f;
    bool isActive = true;
    AABB collider;
};

class BulletManager {
private:
    Sphere* bulletMesh = nullptr;
    std::vector<Bullet> bullets;

public:
    void init(Sphere* mesh) {
        bulletMesh = mesh;
    }

    void spawnBullet(Vec3 startPos, Vec3 dir) {
        Bullet b;
        b.position = startPos;
        b.direction = dir;
        b.isActive = true;
        b.speed = 100.0f;

        b.collider.min = b.position - Vec3(0.05f, 0.05f, 0.05f);
        b.collider.max = b.position + Vec3(0.05f, 0.05f, 0.05f);

        bullets.push_back(b);
    }

    void update(float dt, EnemyManager& enemyMgr, const std::vector<AABB>& walls) {
        for (int i = 0; i < bullets.size(); i++) {
            if (!bullets[i].isActive) continue;

            bullets[i].lifeTime -= dt;
            if (bullets[i].lifeTime <= 0.0f) {
                bullets[i].isActive = false;
                continue;
            }

            bullets[i].position += bullets[i].direction * bullets[i].speed * dt;

            Vec3 size(0.1f, 0.1f, 0.1f);
            bullets[i].collider.min = bullets[i].position - (size * 0.5f);
            bullets[i].collider.max = bullets[i].position + (size * 0.5f);

            bool hitWall = false;
            for (const auto& wall : walls) {
                if (AABB::check(bullets[i].collider, wall)) {
                    hitWall = true;
                    break;
                }
            }
            if (hitWall) {
                bullets[i].isActive = false;
                continue;
            }

            std::vector<Enemy>& enemies = enemyMgr.getEnemies();
            for (auto& enemy : enemies) {
                if (enemy.isDead) continue;

                if (AABB::check(bullets[i].collider, enemy.collider)) {
                    enemy.isDead = true;
                    bullets[i].isActive = false;
                    break;
                }
            }
        }

        std::vector<Bullet> activeBullets;
        for (const auto& b : bullets) {
            if (b.isActive) activeBullets.push_back(b);
        }
        bullets = activeBullets;
    }

    void draw(Core* core, Matrix vp) {
        if (!bulletMesh) return;

        for (const auto& b : bullets) {
            if (!b.isActive) continue;

            Matrix S, T;
            S.scaling(Vec3(0.02f, 0.02f, 0.02f));
            T.translation(b.position);
            Matrix world = S * T;

            bulletMesh->draw(core, world, vp);
        }
    }
};