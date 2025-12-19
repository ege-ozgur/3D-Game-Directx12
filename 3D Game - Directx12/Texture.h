#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string> 
#include <d3d12.h>
#include "Core.h"
#include <iostream>
#include <vector>
#include <direct.h>

using namespace std;

class Texture { // it is a simple texture class to load and upload textures to the GPU using DirectX 12. Written based on the lecture slides.
public:
    ID3D12Resource* tex = nullptr;
    int heapOffset = -1;
    int width = 0;
    int height = 0;
    int channels = 0;

    vector<unsigned char> pixels;

    void load(Core* core, string filename) {
        unsigned char* texels = stbi_load(filename.c_str(), &width, &height, &channels, 0);

        if (!texels) {
            char cwd[1024];
            _getcwd(cwd, sizeof(cwd));

            string fullPath = string(cwd) + "\\" + filename;
			string msg = "Texture NOT FOUND!\n\nProgram looked at:\n" + fullPath + // if the texture is not found, show an error message box with the current working directory. I needed this while testing to figure out where the program was looking for the texture files.
                "\n\nPlease ensure the 'Models' folder is located next to your project files (main.cpp).";

            MessageBoxA(NULL, msg.c_str(), "Texture Error", MB_OK | MB_ICONERROR);
            return;
        }

        if (width <= 0 || height <= 0) {
            stbi_image_free(texels);
            return;
        }

		if (channels == 3) { // if the image has 3 channels (RGB) we convert it to 4 channels (RGBA) by adding an alpha channel with value 255 
            channels = 4;
            unsigned char* texelsWithAlpha = new unsigned char[width * height * channels];
            for (int i = 0; i < (width * height); i++) {
                texelsWithAlpha[i * 4] = texels[i * 3];
                texelsWithAlpha[(i * 4) + 1] = texels[(i * 3) + 1];
                texelsWithAlpha[(i * 4) + 2] = texels[(i * 3) + 2];
                texelsWithAlpha[(i * 4) + 3] = 255;
            }
            upload(core, texelsWithAlpha, width, height, channels);
            delete[] texelsWithAlpha;
        }
        else {
            upload(core, texels, width, height, channels);
        }
        stbi_image_free(texels);
    }

	void upload(Core* core, unsigned char* texels, int _width, int _height, int _channels) { // upload the texture to the GPU

        this->width = _width;
        this->height = _height;
        this->channels = _channels;

        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

        D3D12_HEAP_PROPERTIES heapDesc = {};
        heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = _width;
        textureDesc.Height = _height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = core->device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&tex));

        if (FAILED(hr)) {
            MessageBoxA(NULL, "Failed to create GPU Texture Resource!", "DirectX Error", MB_OK);
            return;
        }

        D3D12_RESOURCE_DESC desc = tex->GetDesc();
        UINT64 textureUploadBufferSize;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

        core->device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, NULL, NULL, &textureUploadBufferSize);

        core->uploadResource(tex, texels, _width * _height * 4,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &footprint);

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = core->srvHeap.getNextCPUHandle();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        core->device->CreateShaderResourceView(tex, &srvDesc, srvHandle);

        heapOffset = core->srvHeap.used - 1;
    }
};