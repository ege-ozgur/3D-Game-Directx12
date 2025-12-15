#pragma once
#include "Core.h"
#include "Mesh.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "Maths.h"
#include "GEMLoader.h"
#include "Vertex.h"
#include "ConstantBuffer.h"
#include "ShaderReflection.h"

#include <vector>
#include <string>
#include <cstdlib> 
#include <ctime>  

struct GrassInstanceData
{
    Matrix world;
};

class Grass
{
private:
    Mesh* grassMesh = nullptr;

    ID3D12Resource* instanceBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW instanceView = {};

    ConstantBuffer* sceneCB = nullptr;

    int numInstances = 0;
    int textureID = -1;
    std::string textureFilename;

    std::string psoName = "GrassPSO";
    std::string vsName = "GrassVS";
    std::string psName = "GrassPS";

public:
    Grass() = default;
    Grass(const Grass&) = delete;
    Grass& operator=(const Grass&) = delete;

    ~Grass()
    {
        if (sceneCB) { delete sceneCB; sceneCB = nullptr; }
        if (grassMesh) { delete grassMesh; grassMesh = nullptr; }
        if (instanceBuffer) { instanceBuffer->Release(); instanceBuffer = nullptr; }
    }

    void init(Core* core,ShaderManager* sm,PSOManager* psoMgr,TextureManager* tm,int count,const std::string& gemPath,float areaHalfSize = 50.0f)
    {
        numInstances = count;

        GEMLoader::GEMModelLoader loader;
        std::vector<GEMLoader::GEMMesh> gemmeshes;
        loader.load(gemPath, gemmeshes);

        if (gemmeshes.empty()) return;

        createGrassMesh(core, gemmeshes[0]);

        std::string texName = gemmeshes[0].material.find("albedo").getValue();
        if (!texName.empty()) {
            textureFilename = texName;
            tm->load(core, textureFilename);
            textureID = tm->find(textureFilename);
        }

        ID3DBlob* vs = sm->loadVS(vsName, "grassVS.hlsl");
        ID3DBlob* ps = sm->loadPS(psName, "grassPS.hlsl");

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

            { "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
        };

        D3D12_INPUT_LAYOUT_DESC inputLayout = {};
        inputLayout.pInputElementDescs = inputElementDescs;
        inputLayout.NumElements = _countof(inputElementDescs);

        D3D12_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterDesc.DepthClipEnable = TRUE;

        psoMgr->createPSO(core, psoName, vs, ps, inputLayout, &rasterDesc);

        ConstantBufferLayout reflectLayout = ShaderReflection::reflect(vs, "SceneConstantBuffer");
        ConstantBufferDescription cbDesc(reflectLayout.name);
        cbDesc.totalSize = reflectLayout.totalSize;
        for (auto& kv : reflectLayout.variables) {
            ConstantBufferVariable var;
            var.offset = kv.second.offset;
            var.size = kv.second.size;
            cbDesc.constantBufferData[kv.first] = var;
        }
        sceneCB = new ConstantBuffer();
        sceneCB->init(core, cbDesc);

        std::srand((unsigned)std::time(nullptr));
        std::vector<GrassInstanceData> instances;
        instances.reserve(numInstances);

        for (int i = 0; i < numInstances; i++)
        {
            float x = ((float)std::rand() / RAND_MAX) * (2.0f * areaHalfSize) - areaHalfSize;
            float z = ((float)std::rand() / RAND_MAX) * (2.0f * areaHalfSize) - areaHalfSize;

            float rotY = ((float)std::rand() / RAND_MAX) * 3.14159f * 2.0f;
            float scale = 0.6f + ((float)std::rand() / RAND_MAX) * 0.8f;

            Matrix S, R, T;
            S.scaling(Vec3(scale, scale, scale));
            R.rotAroundY(rotY);
            T.translation(Vec3(x, 0.0f, z));

            Matrix world = S * R * T;

            Matrix worldTransposed;
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    worldTransposed.m[r * 4 + c] = world.m[c * 4 + r];
                }
            }

            GrassInstanceData data;
            data.world = worldTransposed;

            instances.push_back(data);
        }

        createInstanceBuffer(core, instances);
    }

    void draw(Core* core,
        PSOManager* psoMgr,
        ShaderManager* sm,
        TextureManager* tm,
        const Matrix& vp)
    {
        if (!grassMesh || !instanceBuffer || !sceneCB)
            return;

        auto cl = core->getCommandList();

        psoMgr->bind(core, psoName);

        sceneCB->update("VP", (void*)&vp, sizeof(Matrix));
        cl->SetGraphicsRootConstantBufferView(0, sceneCB->getGPUAddress());

        if (textureID != -1)
            sm->updateTexturePS(core, psName, "tex", textureID);

        D3D12_VERTEX_BUFFER_VIEW views[2];
        views[0] = grassMesh->vbView;
        views[1] = instanceView;

        cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cl->IASetVertexBuffers(0, 2, views);
        cl->IASetIndexBuffer(&grassMesh->ibView);

        cl->DrawIndexedInstanced(grassMesh->numMeshIndices, numInstances, 0, 0, 0);

        sceneCB->next();
    }

    void createGrassMesh(Core* core, const GEMLoader::GEMMesh& gm)
    {
        std::vector<STATIC_VERTEX> v;
        v.reserve(gm.verticesStatic.size());

        for (int i = 0; i < (int)gm.verticesStatic.size(); i++)
        {
            STATIC_VERTEX sv;
            memcpy(&sv, &gm.verticesStatic[i], sizeof(STATIC_VERTEX));
            v.push_back(sv);
        }

        grassMesh = new Mesh();
        grassMesh->init(core, v, gm.indices);
    }

    void createInstanceBuffer(Core* core, const std::vector<GrassInstanceData>& instances)
    {
        const UINT bufferSize = (UINT)(sizeof(GrassInstanceData) * instances.size());

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = bufferSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        core->device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&instanceBuffer)
        );

        void* dataPtr = nullptr;
        instanceBuffer->Map(0, nullptr, &dataPtr);
        memcpy(dataPtr, instances.data(), bufferSize);
        instanceBuffer->Unmap(0, nullptr);

        instanceView.BufferLocation = instanceBuffer->GetGPUVirtualAddress();
        instanceView.StrideInBytes = sizeof(GrassInstanceData);
        instanceView.SizeInBytes = bufferSize;
    }
};