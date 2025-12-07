#pragma once
#include "Core.h"
#include "Mesh.h"
#include "ShaderManager.h"
#include "PSOManager.h"
#include "Vertex.h"
#include "maths.h"
#include <vector>
using namespace std;

using namespace std;

struct CubeConstantBuffer {
    Matrix W;
    Matrix VP;
};

class Cube {
public:
    Mesh mesh;
    ShaderManager shaderMgr;
    PSOManager psoMgr;

    const string vsPath = "vertexShader.hlsl";
    const string psPath = "pixelShader.hlsl";

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
        psoMgr.createPSO(core, "CubePSO", vs, ps, layout);

        Vec3 p0 = Vec3(-1.0f, -1.0f, -1.0f);
        Vec3 p1 = Vec3(1.0f, -1.0f, -1.0f);
        Vec3 p2 = Vec3(1.0f, 1.0f, -1.0f);
        Vec3 p3 = Vec3(-1.0f, 1.0f, -1.0f);
        Vec3 p4 = Vec3(-1.0f, -1.0f, 1.0f);
        Vec3 p5 = Vec3(1.0f, -1.0f, 1.0f);
        Vec3 p6 = Vec3(1.0f, 1.0f, 1.0f);
        Vec3 p7 = Vec3(-1.0f, 1.0f, 1.0f);

        vector<STATIC_VERTEX> vertices;

        vertices.push_back(addVertex(p0, Vec3(0.0f, 0.0f, -1.0f), 0.0f, 1.0f));
        vertices.push_back(addVertex(p1, Vec3(0.0f, 0.0f, -1.0f), 1.0f, 1.0f));
        vertices.push_back(addVertex(p2, Vec3(0.0f, 0.0f, -1.0f), 1.0f, 0.0f));
        vertices.push_back(addVertex(p3, Vec3(0.0f, 0.0f, -1.0f), 0.0f, 0.0f));

        vertices.push_back(addVertex(p5, Vec3(0.0f, 0.0f, 1.0f), 0.0f, 1.0f));
        vertices.push_back(addVertex(p4, Vec3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f));
        vertices.push_back(addVertex(p7, Vec3(0.0f, 0.0f, 1.0f), 1.0f, 0.0f));
        vertices.push_back(addVertex(p6, Vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.0f));

        vertices.push_back(addVertex(p4, Vec3(-1.0f, 0.0f, 0.0f), 0.0f, 1.0f));
        vertices.push_back(addVertex(p0, Vec3(-1.0f, 0.0f, 0.0f), 1.0f, 1.0f));
        vertices.push_back(addVertex(p3, Vec3(-1.0f, 0.0f, 0.0f), 1.0f, 0.0f));
        vertices.push_back(addVertex(p7, Vec3(-1.0f, 0.0f, 0.0f), 0.0f, 0.0f));

        vertices.push_back(addVertex(p1, Vec3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f));
        vertices.push_back(addVertex(p5, Vec3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f));
        vertices.push_back(addVertex(p6, Vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f));
        vertices.push_back(addVertex(p2, Vec3(1.0f, 0.0f, 0.0f), 0.0f, 0.0f));

        vertices.push_back(addVertex(p3, Vec3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f));
        vertices.push_back(addVertex(p2, Vec3(0.0f, 1.0f, 0.0f), 1.0f, 1.0f));
        vertices.push_back(addVertex(p6, Vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f));
        vertices.push_back(addVertex(p7, Vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f));

        vertices.push_back(addVertex(p4, Vec3(0.0f, -1.0f, 0.0f), 0.0f, 1.0f));
        vertices.push_back(addVertex(p5, Vec3(0.0f, -1.0f, 0.0f), 1.0f, 1.0f));
        vertices.push_back(addVertex(p1, Vec3(0.0f, -1.0f, 0.0f), 1.0f, 0.0f));
        vertices.push_back(addVertex(p0, Vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f));

        vector<unsigned int> indices;

        indices.push_back(0); indices.push_back(1); indices.push_back(2);
        indices.push_back(0); indices.push_back(2); indices.push_back(3);

        indices.push_back(4); indices.push_back(5); indices.push_back(6);
        indices.push_back(4); indices.push_back(6); indices.push_back(7);

        indices.push_back(8); indices.push_back(9); indices.push_back(10);
        indices.push_back(8); indices.push_back(10); indices.push_back(11);

        indices.push_back(12); indices.push_back(13); indices.push_back(14);
        indices.push_back(12); indices.push_back(14); indices.push_back(15);

        indices.push_back(16); indices.push_back(17); indices.push_back(18);
        indices.push_back(16); indices.push_back(18); indices.push_back(19);

        indices.push_back(20); indices.push_back(21); indices.push_back(22);
        indices.push_back(20); indices.push_back(22); indices.push_back(23);

        mesh.init(core, vertices, indices);
    }

    void draw(Core* core, Matrix world, Matrix vp) {
        psoMgr.bind(core, "CubePSO");

        CubeConstantBuffer cbData;
        cbData.W = world;
        cbData.VP = vp;

        ConstantBuffer* cb = psoMgr.getVSConstantBuffer("CubePSO", 0);
        if (cb) {
            cb->update("W", &cbData.W, sizeof(Matrix));
            cb->update("VP", &cbData.VP, sizeof(Matrix));
        }
        psoMgr.apply(core, "CubePSO");

        mesh.draw(core);
    }
};