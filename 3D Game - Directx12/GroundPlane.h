#pragma once
#include <vector>
#include <string>
#include "Mesh.h"
#include "GEMLoader.h"
#include "Core.h"
#include "Vertex.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "ShaderReflection.h"

using namespace std;

struct GroundConstantBuffer {
    Matrix W;
    Matrix VP;
};

class GroundPlane {
public:
    vector<Mesh*> meshes;
    int albedoID = -1;
    int normalID = -1;

    ShaderManager shaderMgr;
    PSOManager psoMgr;
    ConstantBuffer* cBuffer = nullptr;

    const std::string vsPath = "groundVS.hlsl";
    const std::string psPath = "groundPS.hlsl";

    GroundPlane() = default;
    GroundPlane(const GroundPlane&) = delete;
    GroundPlane& operator=(const GroundPlane&) = delete;

    ~GroundPlane() {
        if (cBuffer) delete cBuffer;
        for (auto m : meshes) delete m;
    }
    void init(Core* core, std::string gemFilename, TextureManager* textureMgr) {

        ID3DBlob* vs = shaderMgr.loadVS("groundVS", vsPath);
        ID3DBlob* ps = shaderMgr.loadPS("groundPS", psPath);

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_INPUT_LAYOUT_DESC layout = {};
        layout.pInputElementDescs = inputElementDescs;
        layout.NumElements = _countof(inputElementDescs);

        psoMgr.createPSO(core, "GroundPSO", vs, ps, layout);

        ConstantBufferLayout reflectLayout = ShaderReflection::reflect(vs, "ConstantBuffer");
        ConstantBufferDescription cbDesc(reflectLayout.name);
        cbDesc.totalSize = reflectLayout.totalSize;

        for (auto& kv : reflectLayout.variables) {
            ConstantBufferVariable var;
            var.offset = kv.second.offset;
            var.size = kv.second.size;
            cbDesc.constantBufferData[kv.first] = var;
        }

        cBuffer = new ConstantBuffer();
        cBuffer->init(core, cbDesc);

        GEMLoader::GEMModelLoader loader;
        vector<GEMLoader::GEMMesh> gemmeshes;
        loader.load(gemFilename, gemmeshes);

        for (int i = 0; i < gemmeshes.size(); i++) {
            Mesh* mesh = new Mesh();
            std::vector<STATIC_VERTEX> vertices;

            for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++) {
                STATIC_VERTEX v;
                v.pos = Vec3(gemmeshes[i].verticesStatic[j].position.x, gemmeshes[i].verticesStatic[j].position.y, gemmeshes[i].verticesStatic[j].position.z);
                v.normal = Vec3(gemmeshes[i].verticesStatic[j].normal.x, gemmeshes[i].verticesStatic[j].normal.y, gemmeshes[i].verticesStatic[j].normal.z);
                v.tangent = Vec3(1.0f, 0.0f, 0.0f);
                v.tu = gemmeshes[i].verticesStatic[j].u;
                v.tv = gemmeshes[i].verticesStatic[j].v;

                vertices.push_back(v);
            }

            mesh->init(core, vertices, gemmeshes[i].indices);
            meshes.push_back(mesh);


            string autoTexPath = gemmeshes[i].material.find("albedo").getValue();

            if (!autoTexPath.empty()) {
                textureMgr->load(core, autoTexPath);
                albedoID = textureMgr->find(autoTexPath);
            }
        }
    }

    void draw(Core* core, Matrix world, Matrix vp, TextureManager* textureMgr) {
        psoMgr.bind(core, "GroundPSO");

        if (cBuffer) {
            cBuffer->update("W", &world, sizeof(Matrix));
            cBuffer->update("VP", &vp, sizeof(Matrix));
            core->getCommandList()->SetGraphicsRootConstantBufferView(0, cBuffer->getGPUAddress());
        }

        if (albedoID != -1) {
            shaderMgr.updateTexturePS(core, "groundPS", "albedoMap", albedoID);
        }

        for (auto m : meshes) {
            m->draw(core);
        }

        if (cBuffer) cBuffer->next();
    }
};