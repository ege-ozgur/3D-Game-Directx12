#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <string>
#include <unordered_map>
#include <map> 
#include <fstream>
#include <sstream>

using namespace std;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

class ShaderManager {
public:
    std::unordered_map<std::string, ID3DBlob*> shaders;
    std::unordered_map<std::string, std::map<std::string, int>> shaderBindMaps;

    ~ShaderManager() {
        for (auto& p : shaders) {
            if (p.second) p.second->Release();
        }
    }

    string loadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            OutputDebugStringA(("Cannot open shader: " + filename + "\n").c_str());
            return "";
        }

        stringstream buffer;
        buffer << file.rdbuf();
        string code = buffer.str();

        if (code.size() >= 3 &&
            (unsigned char)code[0] == 0xEF &&
            (unsigned char)code[1] == 0xBB &&
            (unsigned char)code[2] == 0xBF)
        {
            code.erase(0, 3);
        }

        return code;
    }

    ID3DBlob* compile(const std::string& name, const std::string& file, const std::string& entry, const std::string& target) {
        std::string code = loadFile(file);
        if (code.empty()) return nullptr;

        ID3DBlob* blob = nullptr;
        ID3DBlob* error = nullptr;

        HRESULT hr = D3DCompile(
            code.c_str(), code.size(),
            nullptr, nullptr, nullptr,
            entry.c_str(), target.c_str(),
            D3DCOMPILE_DEBUG, 0,
            &blob, &error
        );

        if (FAILED(hr)) {
            if (error)
                OutputDebugStringA((char*)error->GetBufferPointer());
            return nullptr;
        }

        ID3D12ShaderReflection* reflection = nullptr;
        D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&reflection));

        if (reflection)
        {
            D3D12_SHADER_DESC desc;
            reflection->GetDesc(&desc);

            std::map<std::string, int> localBindPoints;

            for (unsigned int i = 0; i < desc.BoundResources; i++)
            {
                D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                reflection->GetResourceBindingDesc(i, &bindDesc);

                if (bindDesc.Type == D3D_SIT_TEXTURE)
                {
                    localBindPoints.insert({ bindDesc.Name, bindDesc.BindPoint });
                }
            }

            shaderBindMaps[name] = localBindPoints;

            reflection->Release();
        }

        return blob;
    }

    ID3DBlob* loadVS(const std::string& name, const std::string& path) {
        if (shaders.count(name))
            return shaders[name];

        return shaders[name] = compile(name, path, "VS", "vs_5_0");
    }

    ID3DBlob* loadPS(const std::string& name, const std::string& path) {
        if (shaders.count(name))
            return shaders[name];

        return shaders[name] = compile(name, path, "PS", "ps_5_0");
    }

    void updateTexturePS(Core* core, std::string shaderName, std::string textureName, int textureHeapIndex) {
        int bindPoint = getBindPoint(shaderName, textureName);

        if (textureHeapIndex == -1) {
            return;
        }

        if (bindPoint == -1) {
            OutputDebugStringA(("Texture bind point not found: " + textureName + "\n").c_str());
            return;
        }

        if (core->srvHeap.heap == nullptr) {
            return;
        }


        D3D12_GPU_DESCRIPTOR_HANDLE handle = core->srvHeap.gpuHandle;

        long long offsetIndex = (long long)textureHeapIndex - (long long)bindPoint;

        handle.ptr += offsetIndex * core->srvHeap.incrementSize;
        core->getCommandList()->SetGraphicsRootDescriptorTable(2, handle);
    }

    int getBindPoint(const std::string& shaderName, const std::string& textureName) {
        if (shaderBindMaps.count(shaderName) && shaderBindMaps[shaderName].count(textureName)) {
            return shaderBindMaps[shaderName][textureName];
        }
        return -1;
    }
};