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

struct StaticMeshConstantBuffer { // constant buffer structure for static mesh shaders
    Matrix W;
    Matrix VP;
};

class StaticMesh { // static mesh class to load and render static models from GEM files
public:
    vector<Mesh*> meshes;
    vector<string> textureFilenames;

    ShaderManager shaderMgr;
    PSOManager psoMgr;

	// we load the shaders for static mesh rendering
    const string vsPath = "vertexShader.hlsl";
    const string psPath = "pixelShader.hlsl";

    StaticMesh() = default;

    StaticMesh(const StaticMesh&) = delete;
    StaticMesh& operator=(const StaticMesh&) = delete;

    ~StaticMesh() {
        for (auto m : meshes) {
            delete m;
        }
    }

	void init(Core* core, string filename, TextureManager* textureMgr) { // initialize the static mesh from a GEM file

		ID3DBlob* vs = shaderMgr.loadVS("staticVS", vsPath); // load the shaders
        ID3DBlob* ps = shaderMgr.loadPS("staticPS", psPath);

        D3D12_INPUT_LAYOUT_DESC layout = VertexLayoutCache::getStaticLayout();

        D3D12_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE; 
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.MultisampleEnable = FALSE;
        rasterDesc.AntialiasedLineEnable = FALSE;
        rasterDesc.ForcedSampleCount = 0;
        rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		psoMgr.createPSO(core, "StaticMeshPSO", vs, ps, layout, &rasterDesc); // create the PSO

        GEMLoader::GEMModelLoader loader;
        vector<GEMLoader::GEMMesh> gemmeshes;
        loader.load(filename, gemmeshes);

		for (int i = 0; i < gemmeshes.size(); i++) { // create a mesh for each GEM mesh
            Mesh* mesh = new Mesh();
            std::vector<STATIC_VERTEX> vertices;

            for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++) {
                STATIC_VERTEX v;
                memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
                vertices.push_back(v);
            }

			mesh->init(core, vertices, gemmeshes[i].indices); // initialize the mesh with vertices and indices
            meshes.push_back(mesh);

			string texName = gemmeshes[i].material.find("albedo").getValue(); // find the albedo texture from the GEM material
            textureFilenames.push_back(texName);

            if (!texName.empty()) {
				textureMgr->load(core, texName); // if texture exists we load it
            }
        }
    }

	void draw(Core* core, Matrix world, Matrix vp, TextureManager* textureMgr) { // draw the static mesh with given world and view-projection matrices
		psoMgr.bind(core, "StaticMeshPSO"); // we bind the PSO

        StaticMeshConstantBuffer cbData;
        cbData.W = world;
        cbData.VP = vp;

		ConstantBuffer* cb = psoMgr.getVSConstantBuffer("StaticMeshPSO", 0); // get the constant buffer for the vertex shader
		if (cb) { // if constant buffer exists we update it with the matrices
            cb->update("W", &cbData.W, sizeof(Matrix));
            cb->update("VP", &cbData.VP, sizeof(Matrix));
        }

		psoMgr.apply(core, "StaticMeshPSO"); // apply the PSO settings

		for (int i = 0; i < meshes.size(); i++) // drawing of each mesh
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