#pragma once
#include "Sphere.h" 
#include "Maths.h"
#include "Collision.h"
#include "EnemyManager.h"
#include <vector>
#include <algorithm> 

using namespace std;

struct Bullet {
    Vec3 position;
    Vec3 direction;
    float speed = 100.0f;
    float lifeTime = 3.0f;
    bool isActive = true;
};

class BulletManager {
private:
    Sphere* bulletMesh = nullptr;
    vector<Bullet> bullets;

public:
    void init(Sphere* mesh) {
        bulletMesh = mesh;
    }

    void reset() {
        bullets.clear();
    }

    void spawnBullet(Vec3 startPos, Vec3 dir) {
        Bullet b;
        b.position = startPos;
        b.direction = dir;
        b.isActive = true;
        b.speed = 150.0f; 
        bullets.push_back(b);
    }

    void update(float dt, EnemyManager& enemyMgr, const vector<AABB>& walls) {
        for (int i = 0; i < bullets.size(); i++) {
            if (!bullets[i].isActive) continue;

            bullets[i].lifeTime -= dt;
            if (bullets[i].lifeTime <= 0.0f) {
                bullets[i].isActive = false;
                continue;
            }

            float moveDist = bullets[i].speed * dt;

            Ray ray(bullets[i].position, bullets[i].direction);

            float closestHit = moveDist; 
            bool hitSomething = false;
            Enemy* hitEnemy = nullptr;


            for (const auto& wall : walls) {
                float t = 0.0f;
                if (wall.rayAABB(ray, t)) {
                    if (t < closestHit && t >= 0.0f) {
                        closestHit = t;
                        hitSomething = true;
                        hitEnemy = nullptr; 
                    }
                }
            }

            vector<Enemy*>& enemies = enemyMgr.getEnemies();
            for (auto enemy : enemies) {
                if (enemy->isDead) continue;

                float t = 0.0f;
                if (enemy->collider.rayAABB(ray, t)) {
                    if (t < closestHit && t >= 0.0f) {
                        closestHit = t;
                        hitSomething = true;
                        hitEnemy = enemy;
                    }
                }
            }

            if (hitSomething) {
                bullets[i].position += bullets[i].direction * closestHit;
                bullets[i].isActive = false; 

                if (hitEnemy) {
                    hitEnemy->takeDamage(50.0f); 
                }
            }
            else {
                bullets[i].position += bullets[i].direction * moveDist;
            }
        }

        vector<Bullet> activeBullets;
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
            S.scaling(Vec3(0.05f, 0.05f, 0.05f));
            T.translation(b.position);
            Matrix world = S * T;

            bulletMesh->draw(core, world, vp);
        }
    }
};