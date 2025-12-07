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

class AnimatedMesh
{
public:
    std::vector<Mesh*> meshes;
    Animation animation;
    std::vector<std::string> textureFilenames;
    ConstantBuffer* cBuffer = nullptr;

    ~AnimatedMesh() {
        if (cBuffer) delete cBuffer;
        for (auto m : meshes) delete m;
    }

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

        ID3DBlob* vsBlob = shaderMgr->loadVS("AnimatedUntexturedVS", "animVertexShader.hlsl");
        ID3DBlob* psBlob = shaderMgr->loadPS("AnimatedUntexturedPS", "pixelShader.hlsl");

        psos->createPSO(core, "AnimatedModelPSO", vsBlob, psBlob, VertexLayoutCache::getAnimatedLayout());

        ConstantBufferLayout reflectLayout = ShaderReflection::reflect(vsBlob, "staticMeshBuffer");

        ConstantBufferDescription cbDesc(reflectLayout.name);
        cbDesc.totalSize = reflectLayout.totalSize;

        for (auto& kv : reflectLayout.variables)
        {
            ConstantBufferVariable var;
            var.offset = kv.second.offset;
            var.size = kv.second.size;
            cbDesc.constantBufferData[kv.first] = var;
        }

        cBuffer = new ConstantBuffer();
        cBuffer->init(core, cbDesc);

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

    void draw(Core* core, PSOManager* psos, ShaderManager* shaderMgr, AnimationInstance* instance, Matrix& vp, Matrix& w)
    {
        psos->bind(core, "AnimatedModelPSO");

        cBuffer->update("W", &w, sizeof(Matrix));
        cBuffer->update("VP", &vp, sizeof(Matrix));

        size_t boneDataSize = sizeof(instance->matrices);
        cBuffer->update("bones", instance->matrices, boneDataSize);

        core->getCommandList()->SetGraphicsRootConstantBufferView(0, cBuffer->getGPUAddress());

        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i]->draw(core);
        }

        cBuffer->next();
    }
};