#pragma once
#include <vector>
#include <string>
#include "Mesh.h"
#include "GEMLoader.h"
#include "Core.h"
#include "Vertex.h"
#include "ShaderManager.h"
#include "PSOManager.h"

using namespace std;

struct StaticMeshConstantBuffer {
    Matrix W;
    Matrix VP;
};

class StaticMesh {
public:
    vector<Mesh*> meshes;
    ShaderManager shaderMgr;
    PSOManager psoMgr;

    const std::string vsPath = "vertexShader.hlsl";
    const std::string psPath = "pixelShader.hlsl";

    void init(Core* core, std::string filename) {
        ID3DBlob* vs = shaderMgr.loadVS("staticVS", vsPath);
        ID3DBlob* ps = shaderMgr.loadPS("staticPS", psPath);

        D3D12_INPUT_LAYOUT_DESC layout = VertexLayoutCache::getStaticLayout();

        psoMgr.createPSO(core, "StaticMeshPSO", vs, ps, layout);

        GEMLoader::GEMModelLoader loader;
        vector<GEMLoader::GEMMesh> gemmeshes;
        loader.load(filename, gemmeshes);

        for (int i = 0; i < gemmeshes.size(); i++) {
            Mesh* mesh = new Mesh();
            std::vector<STATIC_VERTEX> vertices;

            for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++) {
                STATIC_VERTEX v;
                memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
                vertices.push_back(v);
            }

            mesh->init(core, vertices, gemmeshes[i].indices);
            meshes.push_back(mesh);
        }
    }

    void draw(Core* core, Matrix world, Matrix vp) {
        psoMgr.bind(core, "StaticMeshPSO");

        StaticMeshConstantBuffer cbData;
        cbData.W = world;
        cbData.VP = vp;

        ConstantBuffer* cb = psoMgr.getVSConstantBuffer("StaticMeshPSO", 0);
        if (cb) {
            cb->update("W", &cbData.W, sizeof(Matrix));
            cb->update("VP", &cbData.VP, sizeof(Matrix));
        }

        psoMgr.apply(core, "StaticMeshPSO");

        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i]->draw(core);
        }
    }
};