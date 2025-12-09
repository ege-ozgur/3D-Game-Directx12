#pragma once
#include "Animation.h"
#include "Player.h"
#include <string>
#include <map>

enum class PlayerState {
    IDLE,
    RUN,
    FIRE,
    RELOAD
};

class PlayerAnimManager {
private:
    AnimationInstance* targetAnimInstance = nullptr;
    PlayerState currentState = PlayerState::IDLE;

    map<PlayerState, std::string> animMap;
    map<PlayerState, float> durationMap;

    bool isActionActive = false;
    float currentAnimTime = 0.0f;

public:
    void init(AnimationInstance* animInst) {
        targetAnimInstance = animInst;

        animMap[PlayerState::IDLE] = "04 idle";
        animMap[PlayerState::RUN] = "07 run";
        animMap[PlayerState::FIRE] = "08 fire";
        animMap[PlayerState::RELOAD] = "17 reload";

        durationMap[PlayerState::FIRE] = 0.25f;
        durationMap[PlayerState::RELOAD] = 1.8f;

        setAnimation(PlayerState::IDLE);
    }

    bool isCurrentActionFinished() {
        if (!isActionActive) return true;

        if (currentAnimTime >= durationMap[currentState]) {
            return true;
        }
        return false;
    }

    float getCurrentDuration() {
        if (durationMap.find(currentState) != durationMap.end()) {
            return durationMap[currentState];
        }
        return 0.0f;
    }

    void update(float dt, const Player& player) {
        if (!targetAnimInstance) return;

        currentAnimTime += dt;

        if (isActionActive) {
            float maxDuration = durationMap[currentState];
            if (currentAnimTime >= maxDuration) {
                isActionActive = false;
            }
        }

        if (!isActionActive) {
            PlayerState desiredState = PlayerState::IDLE;

            if (player.isReloading) {
                desiredState = PlayerState::RELOAD;
                isActionActive = true;
            }
            else if (player.isFiring) {
                desiredState = PlayerState::FIRE;
                isActionActive = true;
            }
            else if (player.isMoving) {
                desiredState = PlayerState::RUN;
            }

            if (desiredState != currentState) {
                setAnimation(desiredState);
            }
        }

        float animSpeed = dt;

        if (currentState == PlayerState::RELOAD || currentState == PlayerState::FIRE) {
            float maxDur = durationMap[currentState];
            if (currentAnimTime >= maxDur) {
                animSpeed = 0.0f;
                targetAnimInstance->t = maxDur - 0.01f;
            }
        }

        targetAnimInstance->update(targetAnimInstance->usingAnimation, animSpeed);
    }

private:
    void setAnimation(PlayerState newState) {
        currentState = newState;
        currentAnimTime = 0.0f;

        std::string animName = animMap[newState];
        if (targetAnimInstance->usingAnimation != animName) {
            targetAnimInstance->usingAnimation = animName;
            targetAnimInstance->t = 0.0f;
        }
    }
};