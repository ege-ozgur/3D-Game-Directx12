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

using namespace std;

struct StaticMeshConstantBuffer {
    Matrix W;
    Matrix VP;
};

class StaticMesh {
public:
    vector<Mesh*> meshes;
    vector<string> textureFilenames;

    ShaderManager shaderMgr;
    PSOManager psoMgr;

    const std::string vsPath = "vertexShader.hlsl";
    const std::string psPath = "pixelShader.hlsl";

    StaticMesh() = default;

    StaticMesh(const StaticMesh&) = delete;
    StaticMesh& operator=(const StaticMesh&) = delete;

    ~StaticMesh() {
        for (auto m : meshes) delete m;
    }

    void init(Core* core, std::string filename, TextureManager* textureMgr) {

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

            string texName = gemmeshes[i].material.find("albedo").getValue();

            textureFilenames.push_back(texName);

            if (!texName.empty()) {
                textureMgr->load(core, texName);
            }
        }
    }

    void draw(Core* core, Matrix world, Matrix vp, TextureManager* textureMgr) {
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
            if (i < textureFilenames.size()) {
                int textureIndex = textureMgr->find(textureFilenames[i]);

                if (textureIndex != -1) {
                    shaderMgr.updateTexturePS(core, "staticPS", "tex", textureIndex);
                }
            }

            meshes[i]->draw(core);
        }
    }
};