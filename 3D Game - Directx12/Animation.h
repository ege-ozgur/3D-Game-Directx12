#pragma once

#include <string>
#include <vector>
#include <map>

#include "Maths.h"

using namespace std;

struct Bone // A single bone in the skeleton
{
	string name;
	Matrix offset;
	int parentIndex;
};

struct Skeleton // The skeleton holds all the bones
{
	vector<Bone> bones;
	Matrix globalInverse;
	int findBone(std::string name)
	{
		for (int i = 0; i < bones.size(); i++)
		{
			if (bones[i].name == name)
			{
				return i;
			}
		}
		return -1;
	}
};

struct AnimationFrame // A single frame in an animation sequence which holds positions, rotations and scales for all bones
{
	vector<Vec3> positions;
	vector<Quaternion> rotations;
	vector<Vec3> scales;
};

struct AnimationSequence // This holds rescaled times
{
	vector<AnimationFrame> frames;
	float ticksPerSecond;
	Vec3 interpolate(Vec3 p1, Vec3 p2, float t)
	{
		return ((p1 * (1.0f - t)) + (p2 * t));
	}
	Quaternion interpolate(Quaternion q1, Quaternion q2, float t)
	{
		return Quaternion::slerp(q1, q2, t);
	}
	float duration() // gets the duration of the animation in seconds
	{
		return ((float)frames.size() / ticksPerSecond);
	}
	void calcFrame(float t, int& frame, float& interpolationFact) // calculates the current frame and interpolation factor at time t
	{
		interpolationFact = t * ticksPerSecond;
		frame = (int)floorf(interpolationFact);
		interpolationFact = interpolationFact - (float)frame;
		frame = min(frame, (int)(frames.size() - 1));
	}
	bool running(float t) // checks if the animation is still running at time t
	{
		if ((int)floorf(t * ticksPerSecond) < frames.size())
		{
			return true;
		}
		return false;
	}
	int nextFrame(int frame) // gets the next frame index
	{
		return min(frame + 1, (int)(frames.size() - 1));
	}
	Matrix interpolateBoneToGlobal(Matrix* matrices, int baseFrame, float interpolationFact, Skeleton* skeleton, int boneIndex) // this is taken from Tom's code. It basically interpolates the local matrix of a bone and then multiplies it by its parent's global matrix to get the global matrix
	{
		Matrix scale = Matrix::scaling3D(interpolate(frames[baseFrame].scales[boneIndex], frames[nextFrame(baseFrame)].scales[boneIndex], interpolationFact));
		Matrix rotation = interpolate(frames[baseFrame].rotations[boneIndex], frames[nextFrame(baseFrame)].rotations[boneIndex], interpolationFact).toMatrix();
		Matrix translation = Matrix::translation3D(interpolate(frames[baseFrame].positions[boneIndex], frames[nextFrame(baseFrame)].positions[boneIndex], interpolationFact));
		Matrix local = scale * rotation * translation; // the order matters here which is from right to left 
		if (skeleton->bones[boneIndex].parentIndex > -1)
		{
			Matrix global = local * matrices[skeleton->bones[boneIndex].parentIndex];
			return global;
		}
		return local;
	}
};

class Animation // The main animation class which holds all animation sequences and the skeleton
{
public:
	map<string, AnimationSequence> animations;
	Skeleton skeleton;
	int bonesSize() // gets the number of bones in the skeleton
	{
		return skeleton.bones.size();
	}
	void calcFrame(std::string name, float t, int& frame, float& interpolationFact) // calculates the current frame and interpolation factor
	{
		animations[name].calcFrame(t, frame, interpolationFact);
	}
	Matrix interpolateBoneToGlobal(std::string name, Matrix* matrices, int baseFrame, float interpolationFact, int boneIndex)  // interpolates a bone to global space
	{
		return animations[name].interpolateBoneToGlobal(matrices, baseFrame, interpolationFact, &skeleton, boneIndex);
	}
	void calcTransforms(Matrix* matrices, Matrix coordTransform)
	{
		for (int i = 0; i < bonesSize(); i++)
		{
			matrices[i] = skeleton.bones[i].offset * matrices[i] * skeleton.globalInverse * coordTransform;
		}
	}
	bool hasAnimation(string name) // checks if the animation sequence exists
	{
		if (animations.find(name) == animations.end())
		{
			return false;
		}
		return true;
	}
};

class AnimationInstance // An instance of an animation which holds the current state of the animation
{
public:
	Animation* animation;
	string usingAnimation;
	float t;

	bool loop = true;

	Matrix matrices[256];
	Matrix matricesPose[256];
	Matrix coordTransform;

	void init(Animation* _animation, int fromYZX) // fromYZX = 1 if the model is in YZX format and needs to be rotated to XYZ
	{
		animation = _animation;
		loop = true; 
		if (fromYZX == 1)
		{
			coordTransform.rotationX(3.14159f); // we rotate the model -90 degrees around the x-axis to convert from YZX to XYZ
		}
		else
		{
			coordTransform = Matrix(); // we declare it as identity matrix
		}
	}

	void update(string name, float dt) // updates the animation 
	{
		if (name != usingAnimation) // if we are changing the animation 
		{
			usingAnimation = name; // we set the new animation
			t = 0; // we reset the time
		}

		t += dt; // we advance the time

		if (animation->animations.find(usingAnimation) == animation->animations.end()) { // if the animation doesn't exist
			return;
		}
		float duration = animation->animations[usingAnimation].duration(); // we get the duration of the animation
		if (duration <= 0.0f) {
			return;
		}
		float frameTime = t;

		if (t >= duration)
		{
			if (loop)
			{
				t = fmod(t, duration);
				frameTime = t;
			}
			else
			{
				frameTime = duration - 0.001f;

				if (frameTime < 0.0f) {
					frameTime = 0.0f;
				}
			}
		}

		int frame = 0;
		float interpolationFact = 0;

		animation->calcFrame(name, frameTime, frame, interpolationFact); // we calculate the current frame and interpolation factor

		for (int i = 0; i < animation->bonesSize(); i++)
		{
			matrices[i] = animation->interpolateBoneToGlobal(name, matrices, frame, interpolationFact, i); // we calculate the global matrix for each bone
		}
		animation->calcTransforms(matrices, coordTransform);
	}

	void resetAnimationTime()
	{
		t = 0;
	}

	bool animationFinished() // checks if the animation has finished
	{
		if (!loop && t > animation->animations[usingAnimation].duration())
		{
			return true;
		}
		return false;
	}

	Matrix findWorldMatrix(string boneName) // finds the world matrix of a specific bone at the current animation time
	{
		int boneID = animation->skeleton.findBone(boneName);
		vector<int> boneChain;
		int ID = boneID;
		while (ID != -1)
		{
			boneChain.push_back(ID);
			ID = animation->skeleton.bones[ID].parentIndex;
		}
		int frame = 0;
		float interpolationFact = 0;
		float duration = animation->animations[usingAnimation].duration();
		float evalTime = t;
		if (!loop && t > duration) evalTime = duration;
		else if (loop && t > duration) evalTime = fmod(t, duration);

		animation->calcFrame(usingAnimation, evalTime, frame, interpolationFact);

		for (int i = boneChain.size() - 1; i > -1; i = i - 1)
		{
			matricesPose[boneChain[i]] = animation->interpolateBoneToGlobal(usingAnimation, matricesPose, frame, interpolationFact, boneChain[i]);
		}
		return (matricesPose[boneID] * coordTransform);
	}
};