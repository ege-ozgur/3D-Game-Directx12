#pragma once
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <string>
#include <unordered_map>
#include "ConstantBuffer.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

using namespace std;

struct ShaderVariable { // Represents a variable in a constant buffer
    string name;
    UINT offset;
    UINT size;
};

struct ConstantBufferLayout { // Represents the layout of a constant buffer
    string name;
    UINT totalSize;
    unordered_map<string, ShaderVariable> variables;
};

class ShaderReflection { // Shader reflection class to extract constant buffer layouts from compiled shaders
public:
    static ConstantBufferLayout reflect(ID3DBlob* shader, const string& bufferName) {
        ConstantBufferLayout layout;
        layout.name = bufferName;
        layout.totalSize = 0;

        ID3D12ShaderReflection* reflector = nullptr;

        HRESULT hr = D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(),IID_PPV_ARGS(&reflector));

        if (FAILED(hr) || !reflector) {
			return layout;
        }
        D3D12_SHADER_DESC shaderDesc = {};
        reflector->GetDesc(&shaderDesc);

        for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {

            auto cb = reflector->GetConstantBufferByIndex(i);
            D3D12_SHADER_BUFFER_DESC cbDesc = {};
            cb->GetDesc(&cbDesc);

            if (bufferName == cbDesc.Name) {
                layout.totalSize = cbDesc.Size;

                for (UINT v = 0; v < cbDesc.Variables; v++) {
                    auto var = cb->GetVariableByIndex(v);
                    D3D12_SHADER_VARIABLE_DESC vDesc = {};
                    var->GetDesc(&vDesc);

                    layout.variables[vDesc.Name] =
                    { vDesc.Name, vDesc.StartOffset, vDesc.Size };
                }
            }
        }

        reflector->Release();
        return layout;
    }
};
