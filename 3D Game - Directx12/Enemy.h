#pragma once
#include "Maths.h"
#include "AnimatedMesh.h"
#include "Collision.h"

struct Enemy { // this is the enemy structure which holds position, rotation, scale, transform matrix, animation instance, collider, health, and state flags
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;

    Matrix transform;
    AnimationInstance anim;
	AABB collider; // bounding box collider for the enemy

	float health = 100.0f; // the health is 100 so that it takes 2 bullets to kill an enemy as each bullet does 50 damage
    bool isDead = false; // bool to check if enemy is dead or alive
	bool isDying = false; // bool to check if enemy is in dying animation

	void updateTransform() { // this function updates the transform matrix and collider based on position, rotation, and scale
        Matrix S, R, T;
        S.scaling(scale);
        R.rotAroundY(rotation.y);
        T.translation(position);
        transform = S * R * T; // we multiply from right to left

		Vec3 finalSize(0.8f, 3.5f, 0.8f); // The size of the enemy collider by trial and error method
		Vec3 centerPos = position; // we start from the enemy position
		centerPos.y += finalSize.y * 0.5f; // we offset the center position by half the height to align the collider properly

		collider.min = centerPos - (finalSize * 0.5f); // we set the min and max of the collider based on center position and final size
        collider.max = centerPos + (finalSize * 0.5f);
    }

    void takeDamage(float amount) {
		if (isDead || isDying) { // if enemy is already dead or dying we dont't apply any damage
            return;
        }
		health -= amount; // we reduce health by the damage amount which is 50 per bullet
		if (health <= 0.0f) { // if health is zero or below we set it to zero and start dying animation
            health = 0.0f;
            isDying = true; 
            anim.usingAnimation = "death from the front"; // the name of the animation for dying
            anim.t = 0.0f; 
            anim.loop = false;
        }
    }
};