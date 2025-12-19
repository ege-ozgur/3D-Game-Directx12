#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <map>
#include <d3d12.h> 
#include <d3dcompiler.h> 
#include <d3d12shader.h> 
#include "Core.h"
#include "ConstantBuffer.h"

using namespace std;

class PSOManager { // A Pipeline State Object manager to create and store PSOs along with their associated constant buffer layouts and buffers. Code based on the lecture slides.
public:
    unordered_map<string, ID3D12PipelineState*> psos;

    unordered_map<string, vector<ConstantBufferDescription>> vsLayouts;
    unordered_map<string, vector<ConstantBufferDescription>> psLayouts;

    unordered_map<string, vector<ConstantBuffer*>> vsBuffers;
    unordered_map<string, vector<ConstantBuffer*>> psBuffers;

    PSOManager() = default;

    PSOManager(const PSOManager&) = delete;
    PSOManager& operator=(const PSOManager&) = delete;

    void createPSO(Core* core, const string& name, ID3DBlob* vs, ID3DBlob* ps, D3D12_INPUT_LAYOUT_DESC layout, D3D12_RASTERIZER_DESC* customRasterizer = nullptr) {
        if (psos.find(name) != psos.end())
            return;

        if (!vs) {
            OutputDebugStringA("ERROR: Vertex Shader (vs) NULL!\n");
            return;
        }
        if (!ps) {
            OutputDebugStringA("ERROR: Pixel Shader (ps) NULL!\n");
            return;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = layout;
        desc.pRootSignature = core->rootSignature;
        desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
        desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

        if (customRasterizer != nullptr) {
            desc.RasterizerState = *customRasterizer;
        }
        else {
            D3D12_RASTERIZER_DESC rasterDesc = {};
            rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
            rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
            rasterDesc.FrontCounterClockwise = FALSE;
            rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            rasterDesc.DepthClipEnable = TRUE;
            rasterDesc.MultisampleEnable = FALSE;
            rasterDesc.AntialiasedLineEnable = FALSE;
            rasterDesc.ForcedSampleCount = 0;
            rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
            desc.RasterizerState = rasterDesc;
        }

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilDesc.StencilEnable = FALSE;
        desc.DepthStencilState = depthStencilDesc;

        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlend = {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL
        };
        for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
            blendDesc.RenderTarget[i] = defaultRenderTargetBlend;
        desc.BlendState = blendDesc;

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc.Count = 1;

        ID3D12PipelineState* pso = nullptr;
        HRESULT hr = core->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
        if (FAILED(hr))
        {
            OutputDebugStringA("PSOManager::createPSO - CreateGraphicsPipelineState FAILED\n");
            return;
        }

        psos.insert({ name, pso });

        ID3D12ShaderReflection* vsReflection = nullptr;
        ID3D12ShaderReflection* psReflection = nullptr;

        hr = D3DReflect(vs->GetBufferPointer(), vs->GetBufferSize(), IID_PPV_ARGS(&vsReflection));
        if (FAILED(hr)) vsReflection = nullptr;

        hr = D3DReflect(ps->GetBufferPointer(), ps->GetBufferSize(), IID_PPV_ARGS(&psReflection));
        if (FAILED(hr)) psReflection = nullptr;

        vector<ConstantBufferDescription> reflectedVS;
        vector<ConstantBufferDescription> reflectedPS;

        if (vsReflection) {
            D3D12_SHADER_DESC vsDesc = {};
            vsReflection->GetDesc(&vsDesc);
            for (UINT i = 0; i < vsDesc.ConstantBuffers; i++) {
                ID3D12ShaderReflectionConstantBuffer* cb = vsReflection->GetConstantBufferByIndex(i);
                D3D12_SHADER_BUFFER_DESC cbDesc = {};
                cb->GetDesc(&cbDesc);
                ConstantBufferDescription cbDescription(cbDesc.Name);
                cbDescription.totalSize = 0;
                for (UINT j = 0; j < cbDesc.Variables; j++) {
                    ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
                    D3D12_SHADER_VARIABLE_DESC vDesc = {};
                    var->GetDesc(&vDesc);
                    ConstantBufferVariable variable;
                    variable.offset = vDesc.StartOffset;
                    variable.size = vDesc.Size;
                    cbDescription.constantBufferData.insert({ vDesc.Name, variable });
                    unsigned int end = variable.offset + variable.size;
                    if (end > cbDescription.totalSize) cbDescription.totalSize = end;
                }
                reflectedVS.push_back(cbDescription);
            }
        }

        if (psReflection) {
            D3D12_SHADER_DESC psDesc = {};
            psReflection->GetDesc(&psDesc);
            for (UINT i = 0; i < psDesc.ConstantBuffers; i++) {
                ID3D12ShaderReflectionConstantBuffer* cb = psReflection->GetConstantBufferByIndex(i);
                D3D12_SHADER_BUFFER_DESC cbDesc = {};
                cb->GetDesc(&cbDesc);
                ConstantBufferDescription cbDescription(cbDesc.Name);
                cbDescription.totalSize = 0;
                for (UINT j = 0; j < cbDesc.Variables; j++) {
                    ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
                    D3D12_SHADER_VARIABLE_DESC vDesc = {};
                    var->GetDesc(&vDesc);
                    ConstantBufferVariable variable;
                    variable.offset = vDesc.StartOffset;
                    variable.size = vDesc.Size;
                    cbDescription.constantBufferData.insert({ vDesc.Name, variable });
                    unsigned int end = variable.offset + variable.size;
                    if (end > cbDescription.totalSize) cbDescription.totalSize = end;
                }
                reflectedPS.push_back(cbDescription);
            }
        }

        if (vsReflection) vsReflection->Release();
        if (psReflection) psReflection->Release();

        vsLayouts[name] = reflectedVS;
        psLayouts[name] = reflectedPS;

        vector<ConstantBuffer*> vsCBs;
        for (auto& descCB : reflectedVS) {
            ConstantBuffer* cb = new ConstantBuffer();
            cb->init(core, descCB, 1024);
            vsCBs.push_back(cb);
        }

        vector<ConstantBuffer*> psCBs;
        for (auto& descCB : reflectedPS) {
            ConstantBuffer* cb = new ConstantBuffer();
            cb->init(core, descCB, 1024);
            psCBs.push_back(cb);
        }

        vsBuffers[name] = vsCBs;
        psBuffers[name] = psCBs;
    }

    void bind(Core* core, const string& name) {
        auto it = psos.find(name);
        if (it == psos.end() || it->second == nullptr) {
            return;
        }
        core->getCommandList()->SetPipelineState(it->second);
    }

    ConstantBuffer* getVSConstantBuffer(const string& name, size_t index = 0) {
        auto it = vsBuffers.find(name);
        if (it == vsBuffers.end()) {
            return nullptr;
        }
        if (index >= it->second.size()) {
            return nullptr;
        }
        return it->second[index];
    }

    ConstantBuffer* getPSConstantBuffer(const string& name, size_t index = 0) {
        auto it = psBuffers.find(name);
        if (it == psBuffers.end()) {
            return nullptr;
        }
        if (index >= it->second.size()) {
            return nullptr;
        }
        return it->second[index];
    }

    void apply(Core* core, const string& name) {
        auto& vsCBs = vsBuffers[name];
        for (size_t i = 0; i < vsCBs.size(); i++) {
            if (vsCBs[i]) {
                core->getCommandList()->SetGraphicsRootConstantBufferView(0 + (UINT)i, vsCBs[i]->getGPUAddress());
                vsCBs[i]->next();
            }
        }

        auto& psCBs = psBuffers[name];
        for (size_t i = 0; i < psCBs.size(); i++) {
            if (psCBs[i]) {
                core->getCommandList()->SetGraphicsRootConstantBufferView(1 + (UINT)i, psCBs[i]->getGPUAddress());
                psCBs[i]->next();
            }
        }
    }

	~PSOManager() { // Release all PSOs and delete constant buffers
        for (auto& pair : psos) {
            if (pair.second) {
                pair.second->Release();
                pair.second = nullptr;
            }
        }
        psos.clear();

        for (auto& pair : vsBuffers) {
            for (ConstantBuffer* cb : pair.second) {
                if (cb) delete cb;
            }
        }
        vsBuffers.clear();

        for (auto& pair : psBuffers) {
            for (ConstantBuffer* cb : pair.second) {
                if (cb) delete cb;
            }
        }
        psBuffers.clear();
    }
};