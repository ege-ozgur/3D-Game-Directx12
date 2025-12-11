#pragma once
#include "AnimatedMesh.h"
#include "Maths.h"
#include "Collision.h"
#include <vector>
#include <cmath>

struct Enemy {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;

    Matrix transform;
    AnimationInstance anim;
    AABB collider;

    float health = 100.0f;
    bool isDead = false;

    void updateTransform() {
        Matrix S, R, T;
        S.scaling(scale);
        R.rotAroundY(rotation.y);
        T.translation(position);
        transform = S * R * T;

        Vec3 size(1.0f, 2.0f, 1.0f);
        collider.min = position - (size * 0.5f);
        collider.max = position + (size * 0.5f);
    }
};

class EnemyManager {
private:
    AnimatedMesh* modelRef = nullptr;
    std::vector<Enemy> enemies;

public:
    void init(AnimatedMesh* model) {
        modelRef = model;
    }

    void spawnEnemy(Vec3 pos, Vec3 scale) {
        Enemy e;
        e.position = pos;
        e.scale = scale;
        e.rotation = Vec3(0, 0, 0);
        e.health = 100.0f;
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

            e.rotation.y = angle + 3.14159f + 0.5f;

            e.updateTransform();
        }
    }

    void draw(Core* core, PSOManager* pso, ShaderManager* sm, TextureManager* tm, Matrix vp) {
        for (auto& e : enemies) {
            if (e.isDead) 
                continue;
            modelRef->draw(core, pso, sm, tm, &e.anim, vp, e.transform);
        }
    }

    std::vector<Enemy>& getEnemies() {
        return enemies;
    }
};