#pragma once
#include "Core.h"
#include "Sphere.h" 
#include "ShaderManager.h"
#include "PSOManager.h"
#include "TextureManager.h"

using namespace std;

class Skybox { // a simple skybox implementation using a textured sphere.
public:
    Sphere* skyMesh = nullptr;
    int textureID = -1;
	string psoName = "SkyboxPSO"; // PSO name for the skybox
    Skybox() = default;

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

	~Skybox() { // destructor to release resources
        if (skyMesh) { 
            delete skyMesh; skyMesh = nullptr; 
        }
    }

	void init(Core* core, ShaderManager* sm, PSOManager* psoMgr, TextureManager* tm) { // Initialize the skybox by loading the texture, shaders, and creating the sphere mesh.
		string texPath = "Models/Textures/skybox.png"; // we use a png texture for the skybox
		tm->load(core, texPath); // we load the texture
        textureID = tm->find(texPath);

		// loading the shader specific to the skybox
        ID3DBlob* vs = sm->loadVS("SkyVS", "skyVS.hlsl"); 
        ID3DBlob* ps = sm->loadPS("SkyPS", "skyPS.hlsl");

		// defining the input layout for the skybox shaders
        vector<D3D12_INPUT_ELEMENT_DESC> layout = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

		psoMgr->createPSO(core, psoName, vs, ps, { layout.data(), (UINT)layout.size() }); // create the PSO for the skybox

		skyMesh = new Sphere(); // we create a sphere mesh for the skybox
		skyMesh->init(core, 50, 50, 5000.0f); // we initialize the sphere with a large radius to make it bigger than our game environment
    }

	void draw(Core* core, PSOManager* psoMgr, Matrix view, Matrix projection) { // Draw the skybox with the given view and projection matrices.
        if (textureID == -1 || !skyMesh) {
            return;
        }
        auto commandList = core->getCommandList();
        psoMgr->bind(core, psoName);

        Matrix viewCentered = view;

		// we enter the view matrix to eliminate translation effects
        viewCentered.m[12] = 0.0f; viewCentered.m[13] = 0.0f; viewCentered.m[14] = 0.0f;
        viewCentered.m[3] = 0.0f; viewCentered.m[7] = 0.0f; viewCentered.m[11] = 0.0f;

        Matrix scaleM;
		scaleM.scaling(Vec3(-1.0f, -1.0f, -1.0f)); // we invert the skybox to render inside the sphere

        Matrix wvp = scaleM * viewCentered * projection;

		// updating the constant buffer with the WVP matrix
        struct SkyCB { 
            Matrix WVP; 
        };
        SkyCB cbData = { 
            wvp
        };

		ConstantBuffer* cb = psoMgr->getVSConstantBuffer(psoName, 0); // we assume the WVP matrix is in the first constant buffer of the vertex shader
        if (cb) {
			cb->update("WVP", &cbData.WVP, sizeof(Matrix)); // we update the WVP matrix in the constant buffer
        }

        psoMgr->apply(core, psoName);

		if (textureID != -1) { // binding the skybox texture to the pipeline
            D3D12_GPU_DESCRIPTOR_HANDLE handle = core->srvHeap.heap->GetGPUDescriptorHandleForHeapStart();
            UINT handleSize = core->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += (UINT64)textureID * handleSize;
            commandList->SetGraphicsRootDescriptorTable(2, handle);
        }

		skyMesh->draw(core); // drawing the skybox mesh
    }
};