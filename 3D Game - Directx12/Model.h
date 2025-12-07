#pragma once
#include <vector>
#include <string>
#include "Maths.h"
#include "Animation.h"
#include "GEMLoader.h"
#include "Core.h"
#include "Vertex.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "Mesh.h"

using namespace std;

class StaticModel
{
public:
	std::vector<Mesh*> meshes;
	std::vector<std::string> textureFilenames;

	void load(Core* core, std::string filename, ShaderManager* shaderMgr, PSOManager* psos)
	{
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;

		loader.load(filename, gemmeshes);

		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh* mesh = new Mesh();
			std::vector<STATIC_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++)
			{
				STATIC_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);
		}

		ID3DBlob* vs = shaderMgr->loadVS("StaticModelVS", "vertexShader.hlsl");
		ID3DBlob* ps = shaderMgr->loadPS("StaticModelPS", "pixelShader.hlsl");

		psos->createPSO(core, "StaticModelPSO", vs, ps, VertexLayoutCache::getStaticLayout());
	}

	void draw(Core* core, PSOManager* psos, Matrix& w, Matrix& vp)
	{
		psos->bind(core, "StaticModelPSO");

		ConstantBuffer* cb = psos->getVSConstantBuffer("StaticModelPSO", 0);
		if (cb)
		{
			Matrix tW = w.transpose(w);
			Matrix tVP = vp.transpose(vp);

			cb->update("W", &tW, sizeof(Matrix));
			cb->update("VP", &tVP, sizeof(Matrix));
		}

		psos->apply(core, "StaticModelPSO");
		for (int i = 0; i < meshes.size(); i++)
		{
			meshes[i]->draw(core);
		}
	}
};

class AnimatedMesh
{
public:
	std::vector<Mesh*> meshes;
	Animation animation;
	std::vector<std::string> textureFilenames;

	void load(Core* core, std::string filename, PSOManager* psos, ShaderManager* shaderMgr)
	{
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		GEMLoader::GEMAnimation gemanimation;

		loader.load(filename, gemmeshes, gemanimation);

		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh* mesh = new Mesh();
			std::vector<ANIMATED_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesAnimated.size(); j++)
			{
				ANIMATED_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesAnimated[j], sizeof(ANIMATED_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);
		}

		ID3DBlob* vs = shaderMgr->loadVS("AnimatedUntexturedVS", "animVertexShader.hlsl");
		ID3DBlob* ps = shaderMgr->loadPS("AnimatedUntexturedPS", "pixelShader.hlsl");

		psos->createPSO(core, "AnimatedMeshPSO", vs, ps, VertexLayoutCache::getAnimatedLayout());

		memcpy(&animation.skeleton.globalInverse, &gemanimation.globalInverse, 16 * sizeof(float));

		for (int i = 0; i < gemanimation.bones.size(); i++)
		{
			Bone bone;
			bone.name = gemanimation.bones[i].name;
			memcpy(&bone.offset, &gemanimation.bones[i].offset, 16 * sizeof(float));
			bone.parentIndex = gemanimation.bones[i].parentIndex;
			animation.skeleton.bones.push_back(bone);
		}

		for (int i = 0; i < gemanimation.animations.size(); i++)
		{
			std::string name = gemanimation.animations[i].name;
			AnimationSequence aseq;
			aseq.ticksPerSecond = gemanimation.animations[i].ticksPerSecond;
			for (int j = 0; j < gemanimation.animations[i].frames.size(); j++)
			{
				AnimationFrame frame;
				for (int index = 0; index < gemanimation.animations[i].frames[j].positions.size(); index++)
				{
					Vec3 p; Quaternion q; Vec3 s;
					memcpy(&p, &gemanimation.animations[i].frames[j].positions[index], sizeof(Vec3));
					frame.positions.push_back(p);
					memcpy(&q, &gemanimation.animations[i].frames[j].rotations[index], sizeof(Quaternion));
					frame.rotations.push_back(q);
					memcpy(&s, &gemanimation.animations[i].frames[j].scales[index], sizeof(Vec3));
					frame.scales.push_back(s);
				}
				aseq.frames.push_back(frame);
			}
			animation.animations.insert({ name, aseq });
		}
	}

	void updateWorld(PSOManager* psos, Matrix& w)
	{
		ConstantBuffer* cb = psos->getVSConstantBuffer("AnimatedMeshPSO", 0);
		if (cb)
		{
			Matrix tW = w.transpose(w);
			cb->update("W", &tW, sizeof(Matrix));
		}
	}

	void draw(Core* core, PSOManager* psos, AnimationInstance* instance, Matrix& vp, Matrix& w)
	{

		psos->bind(core, "AnimatedMeshPSO");

		ConstantBuffer* cb0 = psos->getVSConstantBuffer("AnimatedMeshPSO", 0);
		if (cb0)
		{
			Matrix tW = w.transpose(w);
			Matrix tVP = vp.transpose(vp);

			cb0->update("W", &tW, sizeof(Matrix));
			cb0->update("VP", &tVP, sizeof(Matrix));
		}
		Matrix transposedBones[256];
		for (int i = 0; i < 256; i++) {
			transposedBones[i] = instance->matrices[i].transpose(instance->matrices[i]);
		}

		if (cb0) cb0->update("bones", transposedBones, sizeof(Matrix) * 256);

		ConstantBuffer* cb1 = psos->getVSConstantBuffer("AnimatedMeshPSO", 1);
		if (cb1) cb1->update("bones", transposedBones, sizeof(Matrix) * 256);

		psos->apply(core, "AnimatedMeshPSO");
		for (int i = 0; i < meshes.size(); i++)
		{
			meshes[i]->draw(core);
		}
	}
};