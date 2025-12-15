#pragma once
#include "AnimatedMesh.h"
#include "Maths.h"
#include "Collision.h"
#include "Enemy.h"
#include <vector>
#include <cmath>

using namespace std;


class EnemyManager {
private:
    AnimatedMesh* modelRef = nullptr;
    vector<Enemy> enemies;

public:
    void init(AnimatedMesh* model) {
        modelRef = model;
    }

    void spawnEnemy(Vec3 pos, Vec3 scale) {
        Enemy e;
        e.position = pos;
        e.scale = scale;
        e.rotation = Vec3(0, 0, 0);
        e.isDead = false;

        e.anim.init(&modelRef->animation, 0);
        e.anim.usingAnimation = "idle";
        e.anim.t = ((float)rand() / RAND_MAX);

        e.updateTransform();
        enemies.push_back(e);
    }

    void update(float dt, Vec3 playerPos) {
        for (auto& e : enemies) {
            if (e.isDead) continue;

            e.anim.update("idle", dt);
            Vec3 dir = playerPos - e.position;
            float angle = atan2(dir.x, dir.z);
            e.rotation.y = angle + 3.14159f + 0.2f;

            e.updateTransform();
        }
    }

    void draw(Core* core, PSOManager* pso, ShaderManager* sm, TextureManager* tm, Matrix vp) {
        for (auto& e : enemies) {
            if (e.isDead) {
                continue;
            }              
            modelRef->draw(core, pso, sm, tm, &e.anim, vp, e.transform);
        }
    }

    vector<Enemy>& getEnemies() {
        return enemies;
    }
};