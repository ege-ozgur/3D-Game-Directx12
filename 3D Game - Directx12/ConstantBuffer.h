#pragma once
#include "Core.h"
#include <d3d12.h>
#include <map>
#include <string>
using namespace std;

struct ConstantBufferVariable
{
	unsigned int offset;
	unsigned int size;
};

class ConstantBufferDescription
{
public:
	string name;
	map<string, ConstantBufferVariable> constantBufferData;
	unsigned int totalSize = 0;

	ConstantBufferDescription() {}

	ConstantBufferDescription(const string& cbName)
		: name(cbName), totalSize(0)
	{
	}
};

class ConstantBuffer
{
public:
    ID3D12Resource* resource = nullptr;
    unsigned char* buffer = nullptr;

    unsigned int cbSizeInBytes = 0;
    unsigned int maxInstances = 0;
    unsigned int offsetIndex = 0;

    ConstantBufferDescription layout;

    ConstantBuffer() {}

    void init(Core* core, const ConstantBufferDescription& desc, unsigned int _maxInstances = 1024) {
        layout = desc;

        cbSizeInBytes = (layout.totalSize + 255) & ~255;
        maxInstances = _maxInstances;
        offsetIndex = 0;

        unsigned int totalBytes = cbSizeInBytes * maxInstances;

        D3D12_HEAP_PROPERTIES heapprops = {};
        heapprops.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapprops.CreationNodeMask = 1;
        heapprops.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC cbDesc = {};
        cbDesc.Width = totalBytes;
        cbDesc.Height = 1;
        cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.DepthOrArraySize = 1;
        cbDesc.MipLevels = 1;
        cbDesc.SampleDesc.Count = 1;
        cbDesc.SampleDesc.Quality = 0;
        cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = core->device->CreateCommittedResource(
            &heapprops,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr))
        {
            OutputDebugStringA("ConstantBuffer::init - CreateCommittedResource FAILED\n");
            return;
        }

        hr = resource->Map(0, nullptr, reinterpret_cast<void**>(&buffer));
        if (FAILED(hr))
        {
            OutputDebugStringA("ConstantBuffer::init - Map FAILED\n");
            buffer = nullptr;
        }
    }

    void update(const string& varName, const void* data, size_t dataSize = 0)
    {
        if (!buffer) return;

        auto it = layout.constantBufferData.find(varName);
        if (it == layout.constantBufferData.end())
        {
            return;
        }

        const ConstantBufferVariable& var = it->second;
        const size_t bytes = dataSize == 0 ? var.size : dataSize;

        unsigned char* dst = buffer + (offsetIndex * cbSizeInBytes) + var.offset;
        memcpy(dst, data, bytes);
    }

    void nextInstance()
    {
        offsetIndex = (offsetIndex + 1) % maxInstances;
    }

    D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const
    {
        if (!resource) return 0;
        return resource->GetGPUVirtualAddress() + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(offsetIndex) * cbSizeInBytes;
    }

    void next()
    {
        offsetIndex++;
        if (offsetIndex >= maxInstances)
            offsetIndex = 0;
    }
};