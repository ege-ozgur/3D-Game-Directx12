#pragma once
#include "Core.h"
#include "Sphere.h" 
#include "ShaderManager.h"
#include "PSOManager.h"
#include "TextureManager.h"

using namespace std;

class Skybox {
public:
    Sphere* skyMesh = nullptr;
    int textureID = -1;
    string psoName = "SkyboxPSO";
    Skybox() = default;

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    ~Skybox() {
        if (skyMesh) { delete skyMesh; skyMesh = nullptr; }
    }

    void init(Core* core, ShaderManager* sm, PSOManager* psoMgr, TextureManager* tm) {
        string texPath = "Models/Textures/skybox.png";
        tm->load(core, texPath);
        textureID = tm->find(texPath);

        ID3DBlob* vs = sm->loadVS("SkyVS", "skyVS.hlsl");
        ID3DBlob* ps = sm->loadPS("SkyPS", "skyPS.hlsl");

        vector<D3D12_INPUT_ELEMENT_DESC> layout = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        psoMgr->createPSO(core, psoName, vs, ps, { layout.data(), (UINT)layout.size() });

        skyMesh = new Sphere();
        skyMesh->init(core, 50, 50, 5000.0f);
    }

    void draw(Core* core, PSOManager* psoMgr, Matrix view, Matrix projection) {
        if (textureID == -1 || !skyMesh) return;

        auto cl = core->getCommandList();
        psoMgr->bind(core, psoName);

        Matrix viewCentered = view;

        viewCentered.m[12] = 0.0f; viewCentered.m[13] = 0.0f; viewCentered.m[14] = 0.0f;
        viewCentered.m[3] = 0.0f; viewCentered.m[7] = 0.0f; viewCentered.m[11] = 0.0f;

        Matrix scaleM;
        scaleM.scaling(Vec3(-1.0f, -1.0f, -1.0f));

        Matrix wvp = scaleM * viewCentered * projection;

        struct SkyCB { Matrix WVP; };
        SkyCB cbData = { wvp };

        ConstantBuffer* cb = psoMgr->getVSConstantBuffer(psoName, 0);
        if (cb) {
            cb->update("WVP", &cbData.WVP, sizeof(Matrix));
        }

        psoMgr->apply(core, psoName);

        if (textureID != -1) {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = core->srvHeap.heap->GetGPUDescriptorHandleForHeapStart();
            UINT handleSize = core->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += (UINT64)textureID * handleSize;
            cl->SetGraphicsRootDescriptorTable(2, handle);
        }

        skyMesh->draw(core);
    }
};