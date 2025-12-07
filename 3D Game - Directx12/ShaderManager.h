#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
using namespace std;
#pragma comment(lib, "d3dcompiler.lib")

class ShaderManager {
public:
    std::unordered_map<std::string, ID3DBlob*> shaders;

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

    ID3DBlob* compile(const std::string& file, const std::string& entry, const std::string& target) {
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

        return blob;
    }

    ID3DBlob* loadVS(const std::string& name, const std::string& path) {
        if (shaders.count(name)) return shaders[name];
        return shaders[name] = compile(path, "VS", "vs_5_0");
    }

    ID3DBlob* loadPS(const std::string& name, const std::string& path) {
        if (shaders.count(name)) return shaders[name];
        return shaders[name] = compile(path, "PS", "ps_5_0");
    }
};
