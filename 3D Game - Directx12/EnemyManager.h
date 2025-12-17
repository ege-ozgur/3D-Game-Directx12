#pragma once
#include "AnimatedMesh.h"
#include "Maths.h"
#include "Collision.h"
#include "Enemy.h"
#include <vector>
#include <cmath>

using namespace std;

class EnemyManager {
public:
    AnimatedMesh* modelRef = nullptr;
    vector<Enemy*> enemies;
    ~EnemyManager() {
        reset();
    }

    void reset() {
        for (auto e : enemies) {
            delete e;
        }
        enemies.clear();
    }

    void init(AnimatedMesh* model) {
        modelRef = model;
    }

    void spawnEnemy(Vec3 pos, Vec3 scale) {
        Enemy* e = new Enemy();
        e->position = pos;
        e->scale = scale;
        e->rotation = Vec3(0, 0, 0);
        e->isDead = false;

        e->anim.init(&modelRef->animation, 0);
        e->anim.usingAnimation = "idle";
        e->anim.t = ((float)rand() / (float)RAND_MAX) * 2.0f;

        e->updateTransform();
        enemies.push_back(e);
    }

    void update(float dt, Vec3 playerPos) {
        for (auto e : enemies) {
            if (e->isDead) continue;
            if (e->isDying) {
                e->anim.update("death from the front", dt);

                if (e->anim.t >= 1.0f) { 
                    e->isDead = true;
                }

                e->updateTransform();
                continue; 
            }

            e->anim.update("idle", dt);

            Vec3 dir = playerPos - e->position;
            float angle = atan2(dir.x, dir.z);
            e->rotation.y = angle + 3.14159f;

            e->updateTransform();
        }
    }

    void draw(Core* core, PSOManager* pso, ShaderManager* sm, TextureManager* tm, Matrix vp) {
        for (auto e : enemies) {
            if (e->isDead) continue;
            modelRef->draw(core, pso, sm, tm, &e->anim, vp, e->transform);
        }
    }

    vector<Enemy*>& getEnemies() {
        return enemies;
    }
};