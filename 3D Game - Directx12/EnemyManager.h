#pragma once
#include "AnimatedMesh.h"
#include "Maths.h"
#include "Collision.h"
#include "Enemy.h"
#include <vector>
#include <cmath>

using namespace std;

class EnemyManager { // this class manages all enemies in the game
public:
    AnimatedMesh* enemyModel = nullptr;
    vector<Enemy*> enemies;
    ~EnemyManager() {
        reset();
    }

	void reset() { // this function deletes all enemies and clears the enemy list
        for (auto enemy : enemies) {
            delete enemy;
        }
        enemies.clear();
    }

    void init(AnimatedMesh* model) { 
        enemyModel = model;
    }

	void spawnEnemy(Vec3 pos, Vec3 scale) { // this function spawns a new enemy at the given position and scale
        Enemy* enemy = new Enemy();
        enemy->position = pos;
        enemy->scale = scale;
        enemy->rotation = Vec3(0, 0, 0);
        enemy->isDead = false;

        enemy->anim.init(&enemyModel->animation, 0);
		enemy->anim.usingAnimation = "idle"; // when they spawn they are in idle animation

        enemy->updateTransform();
        enemies.push_back(enemy);
    }

	void update(float dt, Vec3 playerPos) // this function updates all enemies each frame
    {
        for (auto enemy : enemies)
        {
			if (enemy->isDead) // if enemy is dead we skip updating it
                continue;

			if (enemy->isDying) // if enemy is dying we play death animation
            {
                enemy->anim.update("death from the front", dt);
                if (enemy->anim.animationFinished()) 
                    enemy->isDead = true;
                enemy->updateTransform();
                continue;
            }

            enemy->anim.loop = true;
			enemy->anim.update("idle", dt); // we update the idle animation

			// we want the enemies to face the player
			Vec3 targetDir = playerPos - enemy->position; // we calculate direction to player
			targetDir.y = 0.0f;  // we ignore y-axis for rotation

			targetDir.normalize(); // we normalize the direction

			Vec3 forward(0.0f, 0.0f, 1.0f); // enemy's forward direction

			float dot = forward.Dot(targetDir); // we calculate the dot product

			if (dot > 1.0f) { // we make dot product 1 if it is greater than 1
                dot = 1.0f;
            }
			if (dot < -1.0f) { // we make dot product -1 if it is less than -1
                dot = -1.0f;
            }

			float angle = acos(dot); // we calculate the angle using arccosine of dot product

			Vec3 cross = forward.Cross(targetDir); // we calculate the cross product to determine rotation direction

			if (cross.y < 0.0f) // if y component of cross product is negative we rotate in opposite direction for faster rotation
            {
                angle = -angle;
            }
			enemy->rotation.y = angle + 3.14159f; // we set enemy rotation to face the player (+180 degrees to face the player)aaaaaa

            enemy->updateTransform();
        }
    }


	void draw(Core* core, PSOManager* pso, ShaderManager* sm, TextureManager* tm, Matrix vp) { // this function draws all enemies
        for (auto enemy : enemies) {
			if (enemy->isDead) { // if enemy is dead we skip drawing it
                continue;
            }
            enemyModel->draw(core, pso, sm, tm, &enemy->anim, vp, enemy->transform);
        }
    }

	vector<Enemy*>& getEnemies() { // this function returns the list of enemies
        return enemies;
    }
};