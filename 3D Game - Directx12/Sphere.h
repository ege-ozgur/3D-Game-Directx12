#pragma once
#include "Core.h"
#include "Mesh.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "Vertex.h"
#include "maths.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265359f
#endif

using namespace std;

struct SphereConstantBuffer { // constant buffer structure for sphere shader
    Matrix W;
    Matrix VP;
};

class Sphere { // sphere class to generate sphere mesh and handle its rendering
public:
    Mesh mesh;
    ShaderManager shaderMgr;
    PSOManager psoMgr;

	// we load the shaders for static mesh rendering
    const string vsPath = "vertexShader.hlsl";
    const string psPath = "pixelShader.hlsl";

    Sphere() = default;

    Sphere(const Sphere&) = delete;
    Sphere& operator=(const Sphere&) = delete;

	STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv) // function to create and return a static vertex
    {
        STATIC_VERTEX v;
        v.pos = p;
        v.normal = n;
        v.tangent = Vec3(0, 0, 0);
        v.tu = tu;
        v.tv = tv;
        return v;
    }

	void init(Core* core, int rings, int segments, float radius) { // initialize the sphere mesh with given parameters

		ID3DBlob* vs = shaderMgr.loadVS("staticVS", vsPath); // load the vertex shader
		ID3DBlob* ps = shaderMgr.loadPS("staticPS", psPath); // load the pixel shader
        D3D12_INPUT_LAYOUT_DESC layout = VertexLayoutCache::getStaticLayout();
		psoMgr.createPSO(core, "SpherePSO", vs, ps, layout); // create the PSO for sphere rendering

        vector<STATIC_VERTEX> vertices;
        vector<unsigned int> indices;

		for (int lat = 0; lat <= rings; lat++) { 
            float theta = lat * M_PI / rings;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

			for (int lon = 0; lon <= segments; lon++) { 
                float phi = lon * 2.0f * M_PI / segments;
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);

   
                Vec3 position( 
                    radius * sinTheta * cosPhi, 
                    radius * cosTheta,          
                    radius * sinTheta * sinPhi  
                );

                Vec3 normal = position.normalize(); 

                float tu = 1.0f - (float)lon / segments;
                float tv = 1.0f - (float)lat / rings;

                vertices.push_back(addVertex(position, normal, tu, tv));
            }
        }

		// here it generates indices
        for (int lat = 0; lat < rings; lat++)
        {
            for (int lon = 0; lon < segments; lon++)
            {
                int current = lat * (segments + 1) + lon;
                int next = current + segments + 1;

                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);

                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }

        mesh.init(core, vertices, indices);
    }

	void draw(Core* core) { // draw function to render the sphere with default world and view-projection matrices
        mesh.draw(core);
    }

	void draw(Core* core, Matrix world, Matrix vp) { // draw function to render the sphere with given world and view-projection matrices
        psoMgr.bind(core, "SpherePSO");
        SphereConstantBuffer cbData;
        cbData.W = world;
        cbData.VP = vp;

        ConstantBuffer* cb = psoMgr.getVSConstantBuffer("SpherePSO", 0);
        if (cb) {
            cb->update("W", &cbData.W, sizeof(Matrix));
            cb->update("VP", &cbData.VP, sizeof(Matrix));
        }

        psoMgr.apply(core, "SpherePSO");

        mesh.draw(core);
    }
};