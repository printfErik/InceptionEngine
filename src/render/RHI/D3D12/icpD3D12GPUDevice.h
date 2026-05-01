#pragma once

#include "../icpGPUDevice.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <functional>
#include <utility>

INCEPTION_BEGIN_NAMESPACE

class icpD3D12GPUDevice;

class icpD3D12Buffer : public icpRHIBuffer
{
public:
	icpD3D12Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource, uint64_t size);

	void* Map() override;
	void Unmap() override;
	uint64_t GetGPUAddress() const override;
	uint64_t GetSize() const override;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	uint64_t m_size = 0;
};

class icpD3D12Texture : public icpRHITexture
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtv{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsv{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu{};
	bool m_hasRTV = false;
	bool m_hasDSV = false;
	bool m_hasSRV = false;
};

class icpD3D12Sampler : public icpRHISampler
{
};

class icpD3D12Pipeline : public icpRHIPipeline
{
public:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	icpPipelineKind m_kind = icpPipelineKind::GBUFFER;
};

class icpD3D12GPUDevice : public icpRHIDevice
{
public:
	icpD3D12GPUDevice() = default;
	~icpD3D12GPUDevice() override;

	bool Initialize(std::shared_ptr<icpWindowSystem> windowSystem) override;
	void WaitIdle() override;
	void BeginFrame() override;
	void EndFrame() override;
	void ResizeSwapchain() override;

	std::shared_ptr<icpRHIBuffer> CreateBuffer(
		const icpRHIBufferDesc& desc,
		const void* initialData = nullptr) override;
	std::shared_ptr<icpRHITexture> CreateTexture(
		const icpRHITextureDesc& desc,
		const void* initialData = nullptr,
		size_t initialDataSize = 0) override;
	std::shared_ptr<icpRHISampler> CreateSampler() override;
	std::shared_ptr<icpRHIPipeline> CreateGraphicsPipeline(
		const icpGraphicsPipelineDesc& desc) override;

	uint32_t GetCurrentFrameIndex() const override;
	uint32_t GetBackBufferWidth() const override;
	uint32_t GetBackBufferHeight() const override;

	ID3D12Device* GetDevice() const;
	ID3D12GraphicsCommandList* GetCommandList() const;
	ID3D12CommandQueue* GetGraphicsQueue() const;
	IDXGISwapChain3* GetSwapchain() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
	ID3D12Resource* GetCurrentBackBuffer() const;
	ID3D12DescriptorHeap* GetShaderVisibleHeap() const;

	D3D12_CPU_DESCRIPTOR_HANDLE AllocateRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDSV();
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> AllocateSRV();
	D3D12_GPU_DESCRIPTOR_HANDLE CreateTextureSRVTable(const std::vector<std::shared_ptr<icpRHITexture>>& textures);

	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& record);

	DXGI_FORMAT ToDXGIFormat(icpFormat format, bool srv = false) const;
	D3D12_RESOURCE_STATES ToD3D12State(icpResourceState state) const;
	D3D12_PRIMITIVE_TOPOLOGY ToD3D12Topology(icpPrimitiveTopology topology) const;

private:
	void CreateFactory();
	void CreateDevice();
	void CreateCommandObjects();
	void CreateDescriptorHeaps();
	void CreateSwapchain();
	void CreateBackBufferRTVs();
	void CreateFenceObjects();
	void MoveToNextFrame();
	void WaitForFrame(uint32_t frameIndex);
	void ReleaseSwapchainResources();

	std::shared_ptr<icpWindowSystem> m_windowSystem;

	Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[MAX_FRAMES_IN_FLIGHT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_backBuffers[MAX_FRAMES_IN_FLIGHT];
	D3D12_CPU_DESCRIPTOR_HANDLE m_backBufferRTVs[MAX_FRAMES_IN_FLIGHT]{};

	HANDLE m_fenceEvent = nullptr;
	uint64_t m_fenceValues[MAX_FRAMES_IN_FLIGHT]{};
	uint32_t m_currentFrame = 0;

	uint32_t m_backBufferWidth = 1;
	uint32_t m_backBufferHeight = 1;

	uint32_t m_rtvDescriptorSize = 0;
	uint32_t m_dsvDescriptorSize = 0;
	uint32_t m_srvDescriptorSize = 0;
	uint32_t m_nextRTV = MAX_FRAMES_IN_FLIGHT;
	uint32_t m_nextDSV = 0;
	uint32_t m_nextSRV = 0;
};

INCEPTION_END_NAMESPACE
