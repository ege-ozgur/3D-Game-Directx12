#pragma once
#include "Core.h"
#include "Mesh.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "Vertex.h"
#include <vector>

using namespace std;

class Crosshair { // we put a crosshair to the center of the screen for aiming
public:
    Mesh mesh;
    ShaderManager shaderMgr;
    PSOManager psoMgr;

	const string vsPath = "crosshairVS.hlsl"; // Vertex Shader path for crosshair
	const string psPath = "crosshairPS.hlsl"; // Pixel Shader path for crosshair

	STATIC_VERTEX addVertex(float x, float y) // helper function to create a vertex
    {
        STATIC_VERTEX v;
        v.pos = Vec3(x, y, 0.0f);
        v.normal = Vec3(0, 0, -1);
        v.tangent = Vec3(0, 0, 0);
        v.tu = 0;
        v.tv = 0;
        return v;
    }

    void init(Core* core) {
		ID3DBlob* vs = shaderMgr.loadVS("crosshairVS", vsPath); // first we load the shaders
        ID3DBlob* ps = shaderMgr.loadPS("crosshairPS", psPath);
        D3D12_INPUT_LAYOUT_DESC layout = VertexLayoutCache::getStaticLayout();

		psoMgr.createPSO(core, "CrosshairPSO", vs, ps, layout); // then we create the PSO for the crosshair

        vector<STATIC_VERTEX> vertices;
        vector<unsigned int> indices;

        float size = 0.02f; 
        float thickness = 0.002f;

		// here we create a simple crosshair shape using two rectangles to form a cross
        vertices.push_back(addVertex(-size, -thickness)); 
        vertices.push_back(addVertex(-size, thickness)); 
        vertices.push_back(addVertex(size, thickness)); 
        vertices.push_back(addVertex(size, -thickness)); 

        vertices.push_back(addVertex(-thickness, -size)); 
        vertices.push_back(addVertex(-thickness, size)); 
        vertices.push_back(addVertex(thickness, size)); 
        vertices.push_back(addVertex(thickness, -size)); 

		// first rectangle
        indices.push_back(0); indices.push_back(1); indices.push_back(2);
        indices.push_back(0); indices.push_back(2); indices.push_back(3);

		// second rectangle
        indices.push_back(4); indices.push_back(5); indices.push_back(6);
        indices.push_back(4); indices.push_back(6); indices.push_back(7);

        mesh.init(core, vertices, indices);
    }

	void draw(Core* core) { // finally we draw the crosshair at the center of the screen
        psoMgr.bind(core, "CrosshairPSO");
        psoMgr.apply(core, "CrosshairPSO");
        mesh.draw(core);
    }
};