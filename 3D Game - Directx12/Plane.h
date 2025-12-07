#pragma once
#include "Core.h"
#include "Mesh.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "Vertex.h"
#include "maths.h"
#include <vector>
using namespace std;

struct PlaneConstantBuffer {
    Matrix W;
    Matrix VP;
};

class Plane {
public:
    Mesh mesh;
    ShaderManager shaderMgr;
    PSOManager psoMgr;

    const std::string vsPath = "vertexShader.hlsl";
    const std::string psPath = "pixelShader.hlsl";

    STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv)
    {
        STATIC_VERTEX v;
        v.pos = p;
        v.normal = n;
        v.tangent = Vec3(0, 0, 0);
        v.tu = tu;
        v.tv = tv;
        return v;
    }

    void init(Core* core) {
        ID3DBlob* vs = shaderMgr.loadVS("staticVS", vsPath);
        ID3DBlob* ps = shaderMgr.loadPS("staticPS", psPath);

        D3D12_INPUT_LAYOUT_DESC layout = VertexLayoutCache::getStaticLayout();
        psoMgr.createPSO(core, "PlanePSO", vs, ps, layout);

        vector<STATIC_VERTEX> vertices;

        vertices.push_back(addVertex(Vec3(-1, 0, -1), Vec3(0, 1, 0), 0, 0)); 
        vertices.push_back(addVertex(Vec3(1, 0, -1), Vec3(0, 1, 0), 1, 0)); 
        vertices.push_back(addVertex(Vec3(-1, 0, 1), Vec3(0, 1, 0), 0, 1)); 
        vertices.push_back(addVertex(Vec3(1, 0, 1), Vec3(0, 1, 0), 1, 1)); 

        std::vector<unsigned int> indices;

        indices.push_back(0); indices.push_back(2); indices.push_back(1);

        indices.push_back(1); indices.push_back(2); indices.push_back(3);

        mesh.init(core, vertices, indices);
    }

    void draw(Core* core, Matrix world, Matrix vp) {
        psoMgr.bind(core, "PlanePSO");

        PlaneConstantBuffer cbData;
        cbData.W = world;
        cbData.VP = vp;

        ConstantBuffer* cb = psoMgr.getVSConstantBuffer("PlanePSO", 0);
        if (cb) {
            cb->update("W", &cbData.W, sizeof(Matrix));
            cb->update("VP", &cbData.VP, sizeof(Matrix));
        }

        psoMgr.apply(core, "PlanePSO");

        mesh.draw(core);
    }
};