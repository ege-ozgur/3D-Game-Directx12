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

struct GroundConstantBuffer { // Matches the shader constant buffer
    Matrix W;
    Matrix VP;
};

// This class is nearly same as the Static mesh class. I created this to add normal mapping for planes but couldn't manage to do it.
class GroundPlane {
public:
    vector<Mesh*> meshes;
	int albedoID = -1; // -1 means no texture

    ShaderManager shaderMgr;
    PSOManager psoMgr;
    ConstantBuffer* cBuffer = nullptr;

	const string vsPath = "groundVS.hlsl"; // it has different shader files to support normal mapping but currently not used.
    const string psPath = "groundPS.hlsl";

    GroundPlane() = default;
    GroundPlane(const GroundPlane&) = delete;
    GroundPlane& operator=(const GroundPlane&) = delete;

    ~GroundPlane() {
        if (cBuffer) {
            delete cBuffer;
        }
        for (auto m : meshes) {
            delete m;
        }
    }

	void init(Core* core, std::string gemFilename, TextureManager* textureMgr) { // Initialize the ground plane from a GEM file

		ID3DBlob* vs = shaderMgr.loadVS("groundVS", vsPath); // load the shaders
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

		psoMgr.createPSO(core, "GroundPSO", vs, ps, layout); // We create the PSO

		ConstantBufferLayout reflectLayout = ShaderReflection::reflect(vs, "ConstantBuffer"); // I reflect the constant buffer layout from the vertex shader
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
            vector<STATIC_VERTEX> vertices;

			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++) { // we convert GEM vertices to our STATIC_VERTEX format
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

			string autoTexPath = gemmeshes[i].material.find("albedo").getValue(); // we try to load the albedo texture if exists
            if (!autoTexPath.empty()) {
                textureMgr->load(core, autoTexPath);
                albedoID = textureMgr->find(autoTexPath);
            }
        }
    }

	void draw(Core* core, Matrix world, Matrix vp, TextureManager* textureMgr) { // Here we draw the ground plane
		psoMgr.bind(core, "GroundPSO"); // we bind the PSO

		if (cBuffer) { // now we update the constant buffer
            cBuffer->update("W", &world, sizeof(Matrix));
            cBuffer->update("VP", &vp, sizeof(Matrix));
            core->getCommandList()->SetGraphicsRootConstantBufferView(0, cBuffer->getGPUAddress());
        }

		if (albedoID != -1) { // if we have an albedo texture we update it in the pixel shader
            shaderMgr.updateTexturePS(core, "groundPS", "albedoMap", albedoID);
        }

		for (auto m : meshes) { // we draw all the meshes
            m->draw(core);
        }

		if (cBuffer) { // we move to the next instance in the constant buffer
            cBuffer->next();
        }
    }
};