#pragma once
#include "Maths.h"
#include "Core.h"
#include "Window.h"
#include "Collision.h"
#include "AnimatedMesh.h"
#include "Enemy.h"
#include <vector>
#include <cmath>

class Player { // our player class which holds position, rotation, velocity, ammo, firing state, reloading state, movement state, health, speed, mouse sensitivity, eye height, and collider size
public:
    Vec3 position;
    Vec3 rotation;
    Vec3 velocity;

	int currentAmmo = 30; // the gun starts with 30 ammo which is a full clip
	int maxClipAmmo = 30; // maximum bullets in a clip is also 30
	int totalAmmo = 90; // total ammo the player has in reserve

	float fireRate = 0.15f; // time between shots in seconds
    float fireTimer = 0.0f;

	bool isFiring = false; // is the player currently firing
	bool isReloading = false; // is the player currently reloading
	bool isMoving = false; // is the player currently moving

	float health = 100.0f; // Player health but it's not important as the player doesn't take any damage as the enemies don't shoot back while they just stand there
	float speed = 10.0f; // player movement speed which found by trial and error method
	float mouseSensitivity = 0.002f; // mouse sensitivity for looking around
	float eyeHeight = 1.7f; // height of the player's eyes from the ground
	Vec3 colliderSize = Vec3(0.6f, 3.5f, 0.6f); // size of the player's collider box calculated by trial and error method

	void init(Vec3 startPos) { // initialize player position, rotation, and velocity
        position = startPos;
        rotation = Vec3(0, 0, 0);
        velocity = Vec3(0, 0, 0);
    }

	void startReload() { // start reloading if not already reloading and has ammo
        if (isReloading || totalAmmo <= 0 || currentAmmo >= maxClipAmmo) { 
            return;
        }
        isReloading = true;
    }

	Vec3 getForward() const { // calculate forward direction vector based on rotation angles
        Vec3 f;
        // we compute the X component using the yaw (rotation.y) and pitch (rotation.x)
        // sin(yaw) moves the vector left/right, cos(pitch) keeps the vector on the correct vertical plane
        f.x = sinf(rotation.y) * cosf(rotation.x);

        // we compute the Y component from the pitch angle
        // the negative sign is used because positive pitch rotates the view downward in our coordinate system
        f.y = -sinf(rotation.x);

        // we compute the Z component using the yaw and pitch
        // cos(yaw) moves the vector forward/backward, cos(pitch) scales it correctly
        f.z = cosf(rotation.y) * cosf(rotation.x);

        // we normalize the vector so its length is 1 and it can be safely used as a direction
        return f.normalize();
    }

	Vec3 getCrosshairTarget(const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies, float maxDist = 1000.0f) { // get the point where the crosshair is aiming at considering walls and enemies
		Vec3 camPos = getCameraPos(); // get camera position
		Vec3 forward = getForward(); // get forward direction
		Vec3 targetPoint = camPos + (forward * maxDist); // we calculate a default target point far away in the forward direction
		float closestDist = maxDist; // initialize closest distance to max distance

		Ray ray(camPos, forward); // we create a ray from camera position in the forward direction

		for (const auto& wall : walls) { // here we check for intersections with walls
            float t = 0.0f;
			if (wall.rayAABB(ray, t)) { // if ray intersects wall
				if (t < closestDist) { // if this intersection is closer than previous closest
					closestDist = t; // we update closest distance
					targetPoint = ray.at(t); // and we update target point to this intersection point
                }
            }
        }

		for (auto enemy : enemies) { // here we check for intersections with enemies
			if (enemy->isDead) { // if enemy is dead we skip it
                continue;
            }
            float t = 0.0f;
			if (enemy->collider.rayAABB(ray, t)) { // if ray intersects enemy collider
				if (t < closestDist) { // if this intersection is closer than previous closest
					closestDist = t; // we update closest distance
					targetPoint = ray.at(t); // and we update target point to this intersection point
                }
            }
        }
        return targetPoint;
    }

	Enemy* checkShooting(const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies, float maxDist = 1000.0f) { // we check if the player is shooting an enemy considering walls
        Vec3 camPos = getCameraPos();
        Vec3 forward = getForward();
        Ray ray(camPos, forward);

		// same process as getCrosshairTarget but we return the hit enemy instead of target point
        float closestWallDist = maxDist;
        for (const auto& wall : walls) {
            float t = 0.0f;
            if (wall.rayAABB(ray, t)) {
                if (t < closestWallDist) closestWallDist = t;
            }
        }

        Enemy* hitEnemy = nullptr;
        float closestEnemyDist = closestWallDist;

        for (auto enemy : enemies) {
            if (enemy->isDead) continue;
            float t = 0.0f;
            if (enemy->collider.rayAABB(ray, t)) {
                if (t < closestEnemyDist) {
                    closestEnemyDist = t;
                    hitEnemy = enemy;
                }
            }
        }
        return hitEnemy;
    }

	// The player complete the reload process by transferring ammo from total to current clip
    void completeReload() {
        isReloading = false;
		int needed = maxClipAmmo - currentAmmo; // we calculate how much ammo is needed to fill the clip
		if (totalAmmo >= needed) { // if we have enough ammo in total to fill the clip
			totalAmmo -= needed; // we reduce total ammo by needed amount
			currentAmmo = maxClipAmmo; // and set current ammo to max clip ammo
        }
		else { // if we don't have enough ammo to fill the clip
			currentAmmo += totalAmmo; // we add whatever is left in total ammo to current ammo
			totalAmmo = 0; // and set total ammo to zero
        }
    }

	AABB getAABB(Vec3 pos) { // Here we get the player's axis-aligned bounding box at a given position
		Vec3 halfSize = colliderSize * 0.5f; // we calculate half size of the collider because AABB is defined by min and max points
        Vec3 min = Vec3(pos.x - halfSize.x, pos.y, pos.z - halfSize.z);
        Vec3 max = Vec3(pos.x + halfSize.x, pos.y + colliderSize.y, pos.z + halfSize.z);
        return AABB(min, max);
    }

    void update(float dt, Window* win, const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies) { // We update player state based on input, movement, firing, and reloading
        if (fireTimer > 0.0f) { // we decrement fire timer
            fireTimer -= dt;
        }
        isFiring = false; //Now we reset firing state
		// We have two ways to start reloading: automatically when out of ammo, or manually by pressing 'R'
        if (currentAmmo <= 0 && totalAmmo > 0 && !isReloading) {
            startReload();
        }   
        if ((GetAsyncKeyState('R') & 0x8000) || win->keys['R']) {
            startReload();
        }
		if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && !isReloading && currentAmmo > 0 && fireTimer <= 0.0f) { // we check for firing input and conditions
            isFiring = true;
			currentAmmo--; // we reduce current ammo by 1 for each shot
			fireTimer = fireRate; // we reset fire timer
        }

		POINT cursorPos; // we get current cursor position by using Windows API
		GetCursorPos(&cursorPos); // get current cursor position
		RECT rect; // get window rectangle by using Windows API again
		GetWindowRect(win->hwnd, &rect); //we get window rectangle
		int centerX = (rect.left + rect.right) / 2; // we calculate the center of the window
        int centerY = (rect.top + rect.bottom) / 2;

		if (GetForegroundWindow() == win->hwnd) { // we only update rotation if our window is in focus
			float deltaX = (float)(cursorPos.x - centerX); // Now we calculate cursor movement delta
			float deltaY = (float)(cursorPos.y - centerY); // and calculate cursor movement delta
			rotation.y += deltaX * mouseSensitivity; // we update yaw rotation based on horizontal mouse movement
			rotation.x += deltaY * mouseSensitivity; // we update pitch rotation based on vertical mouse movement
			if (rotation.x > 1.5f) { // we clamp pitch rotation to prevent flipping
                rotation.x = 1.5f;
            }
			if (rotation.x < -1.5f) { // clamping pitch rotation again
                rotation.x = -1.5f;
            }
			SetCursorPos(centerX, centerY); // we reset cursor position to center of window
        }

		Vec3 forwardFlat; // we calculate flat forward direction (ignoring y component) for movement
		forwardFlat.x = sinf(rotation.y); // we use only yaw for flat forward direction
		forwardFlat.y = 0; // ignore vertical component
		forwardFlat.z = cosf(rotation.y); // we use only yaw for flat forward direction
		forwardFlat.normalize(); // we normalize the flat forward vector
		Vec3 rightFlat = forwardFlat.Cross(Vec3(0, 1, 0)).normalize(); // we calculate flat right direction by crossing flat forward with up vector and normalizing
        Vec3 moveDir(0, 0, 0);

		if (win->keys['W']) { // pressing W moves forward
            moveDir += forwardFlat;
        }
		if (win->keys['S']) { // pressing S moves backward
            moveDir -= forwardFlat;
        }
		if (win->keys['D']) { // pressing D moves right
            moveDir -= rightFlat;
        }
		if (win->keys['A']) { // pressing A moves left
            moveDir += rightFlat;
        }
		if (moveDir.x != 0 || moveDir.z != 0) { // if there is movement input we normalize the move direction
            moveDir = moveDir.normalize();
            isMoving = true;
        }
		else { // if no movement input we set moving state to false
            isMoving = false;
        }

		Vec3 desiredMove = moveDir * speed * dt; // we calculate desired movement vector based on input, speed, and delta time

		Vec3 nextPosX = position; // we check for collisions separately on X and Z axes
		nextPosX.x += desiredMove.x; // simulate movement on X axis
		AABB playerBoxX = getAABB(nextPosX); //we get the player's AABB at the new X position
		bool hitX = false; // bool to track if we hit something on X axis

		for (const auto& box : walls) { // we check for collisions with walls
            if (AABB::check(playerBoxX, box)) { 
                hitX = true; break;
            }
        }
		if (!hitX) { // if no wall collision, check enemies
            for (auto enemy : enemies) {
				if (!enemy->isDead && AABB::check(playerBoxX, enemy->collider)) { 
                    hitX = true; break;
                } // we skip dead enemies
            }
        }
        if (!hitX) {
            position.x += desiredMove.x; // if no collision, we apply the X movement
        }

		// we do the same process for Z axis
        Vec3 nextPosZ = position;
        nextPosZ.z += desiredMove.z;
        AABB playerBoxZ = getAABB(nextPosZ);
        bool hitZ = false;

        for (const auto& box : walls) {
            if (AABB::check(playerBoxZ, box)) { hitZ = true; break; }
        }
        if (!hitZ) {
            for (auto enemy : enemies) {
                if (!enemy->isDead && AABB::check(playerBoxZ, enemy->collider)) { hitZ = true; break; }
            }
        }
        if (!hitZ) position.z += desiredMove.z;
    }

	Vec3 getCameraPos() const { // get camera position by adding eye height to player position
        Vec3 p = position;
        p.y += eyeHeight;
        return p;
    }

	Matrix getViewMatrix() { // calculate view matrix based on eye position and forward direction
        Vec3 eyePos = position;
        eyePos.y += eyeHeight;
        Vec3 target = eyePos + getForward();
        return Matrix::lookAtMatrix(eyePos, target, Vec3(0, 1, 0));
    }

	Matrix getRotationMatrix() const { // get rotation matrix from player's rotation angles
        Matrix rx, ry;
        rx.rotationX(rotation.x);
        ry.rotAroundY(rotation.y);
        return rx * ry;
    }
};