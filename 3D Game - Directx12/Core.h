#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <vector>

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

class Barrier {
public:
	static void add(ID3D12Resource* res, D3D12_RESOURCE_STATES first, D3D12_RESOURCE_STATES second,
		ID3D12GraphicsCommandList4* commandList) {
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Transition.pResource = res;
		rb.Transition.StateBefore = first;
		rb.Transition.StateAfter = second;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);
	}
};

class GPUFence {
public:
	ID3D12Fence* fence;
	HANDLE eventHandle;
	UINT64 value = 0;
	void create(ID3D12Device5* device) {
		device->CreateFence(value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	void signal(ID3D12CommandQueue* queue) {
		queue->Signal(fence, ++value);
	}
	void wait() {
		if (fence->GetCompletedValue() < value) {
			fence->SetEventOnCompletion(value, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
		}
	}
	~GPUFence() {
		CloseHandle(eventHandle);
		fence->Release();
	}
};

class Core {
public:
	IDXGIAdapter1* adapter;
	ID3D12Device5* device;
	ID3D12CommandQueue* graphicsQueue;
	ID3D12CommandQueue* copyQueue;
	ID3D12CommandQueue* computeQueue;
	IDXGISwapChain3* swapchain;
	ID3D12CommandAllocator* graphicsCommandAllocator[2];
	ID3D12GraphicsCommandList4* graphicsCommandList[2];
	ID3D12DescriptorHeap* backbufferHeap;
	ID3D12Resource** backbuffers;
	GPUFence graphicsQueueFence[2];
	ID3D12DescriptorHeap* dsvHeap;
	ID3D12Resource* dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	ID3D12RootSignature* rootSignature;

	~Core()   {
		rootSignature->Release();
		graphicsCommandList[0]->Release();
		graphicsCommandAllocator[0]->Release();
		graphicsCommandList[1]->Release();
		graphicsCommandAllocator[1]->Release();
		swapchain->Release();
		computeQueue->Release();
		copyQueue->Release();
		graphicsQueue->Release();
		device->Release();
	}

	void initialize(HWND hwnd, int _width, int _height)
	{

		IDXGIFactory6* factory = NULL;
		ID3D12Debug1* debug;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		debug->EnableDebugLayer();
		debug->Release();
		CreateDXGIFactory(__uuidof(IDXGIFactory6), (void**)&factory);

		IDXGIAdapter1* adapterf;
		std::vector<IDXGIAdapter1*> adapters;
		int i = 0;
		while (factory->EnumAdapters1(i, &adapterf) != DXGI_ERROR_NOT_FOUND)
		{
			adapters.push_back(adapterf);
			i++;
		}
		long long maxVideoMemory = 0;
		int useAdapterIndex = 0;
		for (int i = 0; i < adapters.size(); i++)
		{
			DXGI_ADAPTER_DESC desc;
			adapters[i]->GetDesc(&desc);
			if (desc.DedicatedVideoMemory > maxVideoMemory)
			{
				maxVideoMemory = desc.DedicatedVideoMemory;
				useAdapterIndex = i;
			}
		}
		adapter = adapters[useAdapterIndex];
		
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));

		D3D12_COMMAND_QUEUE_DESC graphicsQueueDesc = {};
		graphicsQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		device->CreateCommandQueue(&graphicsQueueDesc, IID_PPV_ARGS(&graphicsQueue));
		
		D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
		copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&copyQueue));
		
		D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
		computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&computeQueue));

		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&graphicsCommandAllocator[0]));
		device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&graphicsCommandList[0]));

		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&graphicsCommandAllocator[1]));
		device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&graphicsCommandList[1]));
		
		graphicsCommandList[0]->Close();
		graphicsCommandList[1]->Close();

		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.Width = _width;
		scDesc.Height = _height;
		scDesc.SampleDesc.Count = 1; // MSAA here
		scDesc.SampleDesc.Quality = 0;
		scDesc.BufferCount = 2;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		IDXGISwapChain1* swapChain1;
		factory->CreateSwapChainForHwnd(graphicsQueue, hwnd, &scDesc, NULL, NULL, &swapChain1);
		swapChain1->QueryInterface(&swapchain);
		swapChain1->Release();
		factory->Release();

		D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc = {};
		renderTargetViewHeapDesc.NumDescriptors = scDesc.BufferCount;
		renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		device->CreateDescriptorHeap(&renderTargetViewHeapDesc, IID_PPV_ARGS(&backbufferHeap));

		backbuffers = new ID3D12Resource * [scDesc.BufferCount];
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = backbufferHeap->GetCPUDescriptorHandleForHeapStart();
		unsigned int renderTargetViewDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (unsigned int i = 0; i < 2; i++)
		{
			swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffers[i]));
			device->CreateRenderTargetView(backbuffers[i], nullptr, renderTargetViewHandle);
			renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
		}
		
		graphicsQueueFence[0].create(device);
		graphicsQueueFence[1].create(device);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		memset(&dsvHeapDesc, 0, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
		dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
		D3D12_CLEAR_VALUE depthClearValue = {};
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heapprops = {};
		heapprops.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapprops.CreationNodeMask = 1;
		heapprops.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.Width = _width;
		dsvDesc.Height = _height;
		dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dsvDesc.DepthOrArraySize = 1;
		dsvDesc.MipLevels = 1;
		dsvDesc.SampleDesc.Count = 1;
		dsvDesc.SampleDesc.Quality = 0;
		dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &dsvDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&dsv));
		device->CreateDepthStencilView(dsv, &depthStencilDesc, dsvHandle);
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = (float)_width;
		viewport.Height = (float)_height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = _width;
		scissorRect.bottom = _height;

		D3D12_ROOT_PARAMETER params[2] = {};

		// VS constant buffer (b0)
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[0].Descriptor.ShaderRegister = 0; // b0
		params[0].Descriptor.RegisterSpace = 0;
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		// PS constant buffer (b1)
		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[1].Descriptor.ShaderRegister = 1; // b1
		params[1].Descriptor.RegisterSpace = 0;
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
		rsDesc.NumParameters = 2;
		rsDesc.pParameters = params;
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		ID3DBlob* serialized = nullptr;
		ID3DBlob* error = nullptr;

		D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			&serialized, &error);

		device->CreateRootSignature(
			0,
			serialized->GetBufferPointer(),
			serialized->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature)
		);

		serialized->Release();
	}

	void flushGraphicsQueue() {
		graphicsQueueFence[0].signal(graphicsQueue);
		graphicsQueueFence[0].wait();
	}

	void resetCommandList()
	{
		unsigned int frameIndex = swapchain->GetCurrentBackBufferIndex();
		graphicsCommandAllocator[frameIndex]->Reset();
		graphicsCommandList[frameIndex]->Reset(graphicsCommandAllocator[frameIndex], NULL);
	}

	ID3D12GraphicsCommandList4* getCommandList()
	{
		unsigned int frameIndex = swapchain->GetCurrentBackBufferIndex();
		return graphicsCommandList[frameIndex];
	}

	void runCommandList()
	{
		getCommandList()->Close();
		ID3D12CommandList* lists[] = { getCommandList() };
		graphicsQueue->ExecuteCommandLists(1, lists);
	}

	void beginFrame()
	{
		unsigned int frameIndex = swapchain->GetCurrentBackBufferIndex();
		graphicsQueueFence[frameIndex].wait();
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = backbufferHeap->GetCPUDescriptorHandleForHeapStart();
		unsigned int renderTargetViewDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		renderTargetViewHandle.ptr += frameIndex * renderTargetViewDescriptorSize;
		resetCommandList();
		Barrier::add(backbuffers[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, getCommandList());
		getCommandList()->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, &dsvHandle);
		float color[4];
		color[0] = 0;
		color[1] = 0;
		color[2] = 1.0;
		color[3] = 1.0;
		getCommandList()->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);
		getCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	}

	void finishFrame()
	{
		unsigned int frameIndex = swapchain->GetCurrentBackBufferIndex();
		Barrier::add(backbuffers[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT, getCommandList());
		runCommandList();
		graphicsQueueFence[frameIndex].signal(graphicsQueue);
		swapchain->Present(1, 0);
	}

	void uploadResource(ID3D12Resource* dstResource, const void* data, unsigned int size, D3D12_RESOURCE_STATES targetState, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* texFootprint = NULL) {
		ID3D12Resource* uploadBuffer;
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = size;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
		void* mappeddata = NULL;
		uploadBuffer->Map(0, NULL, &mappeddata);
		memcpy(mappeddata, data, size);
		uploadBuffer->Unmap(0, NULL);
		resetCommandList();
		if (texFootprint != NULL)
		{
			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = uploadBuffer;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = *texFootprint;
			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = dstResource;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = 0;
			getCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
		}
		else
		{
			getCommandList()->CopyBufferRegion(dstResource, 0, uploadBuffer, 0, size);
		}
		Barrier::add(dstResource, D3D12_RESOURCE_STATE_COPY_DEST, targetState, getCommandList());
		runCommandList();
		flushGraphicsQueue();
		uploadBuffer->Release();
	}

	void beginRenderPass()
	{
		getCommandList()->RSSetViewports(1, &viewport);
		getCommandList()->RSSetScissorRects(1, &scissorRect);
		getCommandList()->SetGraphicsRootSignature(rootSignature);
	}

	void createRootSignature() {
		std::vector<D3D12_ROOT_PARAMETER> parameters;
		D3D12_ROOT_PARAMETER rootParameterCBVS;
		rootParameterCBVS.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameterCBVS.Descriptor.ShaderRegister = 0; // Register(b0)
		rootParameterCBVS.Descriptor.RegisterSpace = 0;
		rootParameterCBVS.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		parameters.push_back(rootParameterCBVS);
		D3D12_ROOT_PARAMETER rootParameterCBPS;
		rootParameterCBPS.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameterCBPS.Descriptor.ShaderRegister = 0; // Register(b0)
		rootParameterCBPS.Descriptor.RegisterSpace = 0;
		rootParameterCBPS.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameters.push_back(rootParameterCBPS);
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = parameters.size();
		desc.pParameters = &parameters[0];
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	int frameIndex()
	{
		return swapchain->GetCurrentBackBufferIndex();
	}
};

