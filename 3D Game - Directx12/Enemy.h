#pragma once
#include "Maths.h"
#include "AnimatedMesh.h"
#include "Collision.h"

struct Enemy {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;

    Matrix transform;
    AnimationInstance anim;
    AABB collider;

    float health = 100.0f;
    bool isDead = false;
    bool isDying = false;

    void updateTransform() {
        Matrix S, R, T;
        S.scaling(scale);
        R.rotAroundY(rotation.y);
        T.translation(position);
        transform = S * R * T;

        Vec3 finalSize(0.8f, 3.5f, 0.8f);
        Vec3 centerPos = position;
        centerPos.y += finalSize.y * 0.5f;

        collider.min = centerPos - (finalSize * 0.5f);
        collider.max = centerPos + (finalSize * 0.5f);
    }

    void takeDamage(float amount) {
        if (isDead || isDying) return;

        health -= amount;
        if (health <= 0.0f) {
            health = 0.0f;
            isDying = true; 
            anim.usingAnimation = "death";
            anim.t = 0.0f; 
            anim.loop = false; 
        }
    }
};