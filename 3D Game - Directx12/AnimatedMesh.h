#pragma once
#include <vector>
#include <string>
#include <d3d12.h>
#include "Core.h"
#include "PSOManager.h"
#include "ShaderManager.h" 
#include "Mesh.h"
#include "Animation.h"
#include "GEMLoader.h"
#include "Vertex.h"
#include "ConstantBuffer.h" 
#include "ShaderReflection.h" 
#include "TextureManager.h"

using namespace std;

// animated mesh consisting of multiple sub-meshes and an animation
class AnimatedMesh
{
public:
    vector<Mesh*> meshes; 
    Animation animation;
    vector<std::string> textureFilenames;
    ConstantBuffer* cBuffer = nullptr;

	AnimatedMesh() = default; // default constructor

	// we disable copy constructor and assignment operator as shallow copy might cause double deletion
    AnimatedMesh(const AnimatedMesh&) = delete;
    AnimatedMesh& operator=(const AnimatedMesh&) = delete;

	~AnimatedMesh() { // destructor
        if (cBuffer) {
            delete cBuffer;
        }
        for (auto m : meshes) {
            delete m;
        }
    }

	void load(Core* core, std::string filename, PSOManager* psos, ShaderManager* shaderMgr, TextureManager* textureMgr) { // it loads the model and the animation from a GEM file 
        GEMLoader::GEMModelLoader loader;
        vector<GEMLoader::GEMMesh> gemmeshes;
        GEMLoader::GEMAnimation gemanimation;
		loader.load(filename, gemmeshes, gemanimation); // it reads the GEM file, meshes and animation

        for (int i = 0; i < gemmeshes.size(); i++)
        {
            Mesh* mesh = new Mesh();
            vector<ANIMATED_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesAnimated.size(); j++) // we copy the vertices to the local engine format
            {
                ANIMATED_VERTEX v;
                memcpy(&v, &gemmeshes[i].verticesAnimated[j], sizeof(ANIMATED_VERTEX));
                vertices.push_back(v);
            }

			string texName = gemmeshes[i].material.find("albedo").getValue(); // we get the texture filename from the material which named as albedo
			textureFilenames.push_back(texName); // we store the texture filename to use it later during drawing

			textureMgr->load(core, texName); // we load the texture to the CPU

            mesh->init(core, vertices, gemmeshes[i].indices);
            meshes.push_back(mesh);
        }

		// we create the PSO for animated models using the appropriate shaders and vertex layout for animated models
        ID3DBlob* vsBlob = shaderMgr->loadVS("AnimatedModelVS", "animVertexShader.hlsl");
        ID3DBlob* psBlob = shaderMgr->loadPS("AnimatedModelPS", "animPixelShader.hlsl");

		psos->createPSO(core, "AnimatedModelPSO", vsBlob, psBlob, VertexLayoutCache::getAnimatedLayout()); // we create the PSO for animated models

		ConstantBufferLayout reflectLayout = ShaderReflection::reflect(vsBlob, "staticMeshBuffer"); // we reflect the constant buffer layout from the vertex shader

        ConstantBufferDescription cbDesc(reflectLayout.name);
        cbDesc.totalSize = reflectLayout.totalSize;

        for (auto& kv : reflectLayout.variables)
        {
            ConstantBufferVariable var;
            var.offset = kv.second.offset;
            var.size = kv.second.size;
            cbDesc.constantBufferData[kv.first] = var;
        }

		cBuffer = new ConstantBuffer(); // we create the constant buffer for animated models
        cBuffer->init(core, cbDesc);

        memcpy(&animation.skeleton.globalInverse, &gemanimation.globalInverse, 16 * sizeof(float)); //
		for (int i = 0; i < gemanimation.bones.size(); i++) // we copy the bones to the local engine format
        {
            Bone bone;
            bone.name = gemanimation.bones[i].name;
            memcpy(&bone.offset, &gemanimation.bones[i].offset, 16 * sizeof(float));
            bone.parentIndex = gemanimation.bones[i].parentIndex;
            animation.skeleton.bones.push_back(bone);
        }
		for (int i = 0; i < gemanimation.animations.size(); i++) // we copy the animation sequences to the local engine format
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

	void draw(Core* core, PSOManager* psos, ShaderManager* shaderMgr, TextureManager* textures, AnimationInstance* instance, Matrix& vp, Matrix& w) // it draws the animated mesh
    {
		psos->bind(core, "AnimatedModelPSO"); // we bind the PSO for animated models

		cBuffer->update("W", &w, sizeof(Matrix)); // we update the world matrix
		cBuffer->update("VP", &vp, sizeof(Matrix)); // we update the view-projection matrix

        size_t boneDataSize = sizeof(instance->matrices);
		cBuffer->update("bones", instance->matrices, boneDataSize); // we update the bone matrices

        core->getCommandList()->SetGraphicsRootConstantBufferView(0, cBuffer->getGPUAddress());

		for (int i = 0; i < meshes.size(); i++) // we draw each sub-mesh
        {
            int textureIndex = textures->find(textureFilenames[i]);
            if (textureIndex != -1) {
				shaderMgr->updateTexturePS(core, "AnimatedModelPS", "tex", textureIndex); // we bind the texture to the pixel shader
            }
			meshes[i]->draw(core); // we draw the sub-mesh
        }

		cBuffer->next(); // we move to the next constant buffer instance
    }
};