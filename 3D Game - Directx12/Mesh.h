#pragma once
#include <d3d12.h>
#include <vector>
#include "Core.h"
#include "Vertex.h"
using namespace std;

class Mesh {
public:
    ID3D12Resource* vertexBuffer = nullptr;
    ID3D12Resource* indexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;
    unsigned int numMeshIndices = 0;

    ~Mesh() {
        if (vertexBuffer) vertexBuffer->Release();
        if (indexBuffer) indexBuffer->Release();
    }

    template<typename VERTEX_TYPE>
    void init(Core* core,const std::vector<VERTEX_TYPE>& vertices,const std::vector<unsigned int>& indices) {
        int vBufferSize = sizeof(VERTEX_TYPE) * vertices.size();

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC vDesc = {};
        vDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vDesc.Width = vBufferSize;
        vDesc.Height = 1;
        vDesc.DepthOrArraySize = 1;
        vDesc.MipLevels = 1;
        vDesc.SampleDesc.Count = 1;
        vDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        core->device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &vDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer)
        );

        void* vData = nullptr;
        vertexBuffer->Map(0, nullptr, &vData);
        memcpy(vData, vertices.data(), vBufferSize);
        vertexBuffer->Unmap(0, nullptr);

        vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vbView.StrideInBytes = sizeof(VERTEX_TYPE);
        vbView.SizeInBytes = vBufferSize;

        int iBufferSize = sizeof(unsigned int) * indices.size();

        D3D12_RESOURCE_DESC iDesc = vDesc;
        iDesc.Width = iBufferSize;

        core->device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &iDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&indexBuffer)
        );

        void* iData = nullptr;
        indexBuffer->Map(0, nullptr, &iData);
        memcpy(iData, indices.data(), iBufferSize);
        indexBuffer->Unmap(0, nullptr);

        ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibView.Format = DXGI_FORMAT_R32_UINT;
        ibView.SizeInBytes = iBufferSize;

        numMeshIndices = (int)indices.size();
    }

    void draw(Core* core) {
        auto cmd = core->getCommandList();
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &vbView);
        cmd->IASetIndexBuffer(&ibView);
        cmd->DrawIndexedInstanced(numMeshIndices, 1, 0, 0, 0);
    }
};

inline STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv) {
    STATIC_VERTEX v;
    v.pos = p; v.normal = n; v.tangent = Vec3(0, 0, 0); v.tu = tu; v.tv = tv;
    return v;
}