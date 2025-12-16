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

    bool isDead = false;

    void updateTransform() {
        if (isDead) {
            collider.reset();
            return;
        }

        Matrix S, R, T;
        S.scaling(scale);
        R.rotAroundY(rotation.y);
        T.translation(position);
        transform = S * R * T;
        Vec3 finalSize(0.7f, 3.5f, 0.7f);

        Vec3 centerPos = position;
        centerPos.y += finalSize.y * 0.5f;

        collider.min = centerPos - (finalSize * 0.5f);
        collider.max = centerPos + (finalSize * 0.5f);
    }

    void takeDamage() {
        isDead = true;
    }
};