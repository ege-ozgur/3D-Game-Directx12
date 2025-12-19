#pragma once
#include "Sphere.h" 
#include "Maths.h"
#include "Collision.h"
#include "EnemyManager.h"
#include <vector>
#include <algorithm> 

using namespace std;

struct Bullet { // the bullet structure which holds position, direction, speed, lifetime, and active status
    Vec3 position;
    Vec3 direction;
    float speed = 150.0f;
	float lifeTime = 3.0f; // after 3 seconds the bullet will be deactivated no matter what
	bool isActive = true; // whether the bullet is active or not. it starts as active as it is spawned
};

class BulletManager { // the bullet manager class which handles spawning, updating, and drawing bullets
private:
	Sphere* bulletMesh = nullptr; // we use a sphere mesh for the bullet representation as i didn't use textures for that
    vector<Bullet> bullets;

public:
    void init(Sphere* mesh) {
        bulletMesh = mesh;
    }

    void reset() {
        bullets.clear();
    }

	void spawnBullet(Vec3 startPos, Vec3 dir) { // spawns a bullet at a given position and direction
        Bullet b;
        b.position = startPos;
        b.direction = dir;
        b.isActive = true;
        b.speed = 150.0f; 
        bullets.push_back(b);
    }

	void update(float dt, EnemyManager& enemyMgr, const vector<AABB>& walls) { // updates all bullets and checks for collisions with walls and enemies
        for (int i = 0; i < bullets.size(); i++) {
            if (!bullets[i].isActive) continue;

            bullets[i].lifeTime -= dt;
			if (bullets[i].lifeTime <= 0.0f) { // deactivate bullet if lifetime is over
                bullets[i].isActive = false;
                continue;
            }

			float moveDist = bullets[i].speed * dt; // distance of the bullet will move this frame

			Ray ray(bullets[i].position, bullets[i].direction); // we used a ray to check for collisions

            float closestHit = moveDist; 
            bool hitSomething = false;
            Enemy* hitEnemy = nullptr;

			for (const auto& wall : walls) { // check collision with walls
                float t = 0.0f;
				if (wall.rayAABB(ray, t)) { // if ray intersects walls bounding box
					if (t < closestHit && t >= 0.0f) { // if this hit is closer than previous hits
						closestHit = t; // update closest hit distance
						hitSomething = true; // mark that we hit something
						hitEnemy = nullptr; // we hit a wall, not an enemy
                    }
                }
            }

            vector<Enemy*>& enemies = enemyMgr.getEnemies();
            for (auto enemy : enemies) {
				if (enemy->isDead) {  // skip dead enemies
                    continue;
                }
                float t = 0.0f;
				if (enemy->collider.rayAABB(ray, t)) { // if ray intersects with enemies bounding box
                    if (t < closestHit && t >= 0.0f) {
                        closestHit = t;
                        hitSomething = true;
                        hitEnemy = enemy;
                    }
                }
            }

			if (hitSomething) { // if we hit something we move the bullet to hit point and deactivate it
                bullets[i].position += bullets[i].direction * closestHit;
                bullets[i].isActive = false; 

                if (hitEnemy) {
					hitEnemy->takeDamage(50.0f); // each bullet gives 50 damage to enemy so it takes 2 bullets to kill an enemy with 100 health
                }
            }
            else {
				bullets[i].position += bullets[i].direction * moveDist; // move bullet forward if there is no collision
            }
        }

        vector<Bullet> activeBullets;
		for (const auto& b : bullets) { // we remove inactive bullets from the list to optimize
			if (b.isActive) activeBullets.push_back(b); // we keep only active bullets
        }
        bullets = activeBullets;
    }

	void draw(Core* core, Matrix vp) { // this funcition draws all active bullets
		if (!bulletMesh) { // if there is no bullet mesh we cannot draw anything so it returns
            return;
        }

		for (const auto& b : bullets) { // we iterate through all bullets
			if (!b.isActive) { // we skip inactive bullets
                continue;
            }
			Matrix S, T; // we declare scaling and translation matrices
			S.scaling(Vec3(0.05f, 0.05f, 0.05f)); // we scale down the bullet mesh to be small. this size set by trial and error method
			T.translation(b.position); // we translate the bullet mesh to the bullet position
			Matrix world = S * T; // we create the world matrix by multiplying scaling and translation matrices

			bulletMesh->draw(core, world, vp); // we draw the bullet 
        }
    }
};