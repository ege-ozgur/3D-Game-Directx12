#pragma once
#include "Animation.h"
#include "Player.h"
#include "BulletManager.h" 
#include <string>
#include <map>
#include <vector>

using namespace std;

enum class PlayerState { // different states the player can be in are set here
    IDLE,
    RUN,
    FIRE,
    RELOAD
};

class PlayerAnimManager { // this class manages the player's animations based on their state and actions
public:
	AnimationInstance* targetAnimInstance = nullptr; // pointer to the animation instance of the player model
	BulletManager* bulletManager = nullptr; // pointer to the bullet manager to spawn bullets when firing

	PlayerState currentState = PlayerState::IDLE; // we set the current state to idle

	map<PlayerState, string> animMap; // map to hold animation names for each player state
	map<PlayerState, float> durationMap; // map to hold animation durations for each player state

	bool isActionActive = false; // flag to indicate if an action animation is currently active
    float currentAnimTime = 0.0f;

	void init(AnimationInstance* animInst, BulletManager* bMgr) { // initialize the animation manager with the player's animation instance and bullet manager
        targetAnimInstance = animInst;
        bulletManager = bMgr;

		animMap[PlayerState::IDLE] = "04 idle"; // the name of the animations as per the model's animation file. They are named like first the number then the action.
		animMap[PlayerState::RUN] = "07 run"; // we set the run animation
		animMap[PlayerState::FIRE] = "08 fire"; // the fire animation
		animMap[PlayerState::RELOAD] = "17 reload"; // the reload animation

		durationMap[PlayerState::FIRE] = 0.25f; // duration of the fire animation
		durationMap[PlayerState::RELOAD] = 1.8f; // duration of the reload animation which found by trial and error method

		setAnimation(PlayerState::IDLE); // we start with idle animation
    }

	bool isCurrentActionFinished() { // checks if the current action animation has finished
		if (!isActionActive) { // if no action is active we consider it finished
            return true;
        }
		if (currentAnimTime >= durationMap[currentState]) { // if the current animation time exceeds the duration we consider it finished
            return true;
        }
        return false;
    }

	float getCurrentDuration() { // gets the duration of the current animation state
        if (durationMap.find(currentState) != durationMap.end()) {
            return durationMap[currentState];
        }
        return 0.0f;
    }

	void update(float dt, Player& player, const std::vector<AABB>& walls, const std::vector<Enemy*>& enemies) { // updates the animation based on player actions and state
		if (!targetAnimInstance) { // if no animation instance is set we return early
            return;
        }
        currentAnimTime += dt;

		if (isActionActive) { // if an action animation is active we check if it has finished
			float maxDuration = durationMap[currentState]; // get the max duration for the current state
            if (currentAnimTime >= maxDuration) {
                isActionActive = false;
            }
        }

		if (!isActionActive) { // if no action animation is active we determine the desired state based on player actions
			PlayerState desiredState = PlayerState::IDLE; // default to idle
			//the reason for checking reloading first is that reloading takes precedence over firing and moving
			if (player.isReloading) { // if the player is reloading we set the desired state to reload
                desiredState = PlayerState::RELOAD;
                isActionActive = true;
            }
			else if (player.isFiring) { // if the player is firing we set the desired state to fire
                desiredState = PlayerState::FIRE;
                isActionActive = true;

				Vec3 target = player.getCrosshairTarget(walls, enemies); // get the target point where the crosshair is aiming

				Vec3 muzzleOffset(0.25f, -0.25f, 0.6f); // offset of the muzzle from the camera position
                Matrix rotMat = player.getRotationMatrix();
                Vec3 rotatedOffset = rotMat.mulVec(muzzleOffset);
				Vec3 muzzlePos = player.getCameraPos() + rotatedOffset; // calculate the muzzle position in world space

                Vec3 bulletDir = (target - muzzlePos).normalize();

                if (bulletManager) {
					bulletManager->spawnBullet(muzzlePos, bulletDir); // spawn a bullet at the muzzle position towards the target. muzzle position is not %100 accurate but good enough
                }
            }
			else if (player.isMoving) { // if the player is moving we set the desired state to run. 
                desiredState = PlayerState::RUN;
            }

			if (desiredState != currentState) { // if the desired state is different from the current state we change the animation
                setAnimation(desiredState);
            }
        }

        float animSpeed = dt;

		if (currentState == PlayerState::RELOAD || currentState == PlayerState::FIRE) { // for action animations we clamp the animation time to not exceed the duration
            float maxDur = durationMap[currentState];
            if (currentAnimTime >= maxDur) {
                animSpeed = 0.0f;
                targetAnimInstance->t = maxDur - 0.01f;
            }
        }

        targetAnimInstance->update(targetAnimInstance->usingAnimation, animSpeed);
    }

	void setAnimation(PlayerState newState) { // sets the animation to the specified state
        currentState = newState;
        currentAnimTime = 0.0f;

        string animName = animMap[newState];
		if (targetAnimInstance->usingAnimation != animName) { // if the animation is different from the current one we change it
            targetAnimInstance->usingAnimation = animName;
            targetAnimInstance->t = 0.0f;
        }
    }
};