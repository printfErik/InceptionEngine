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
	D3D12_CPU_DESCRIPTOR_HANDLE m_readOnlyDsv{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_uavGpu{};
	bool m_hasRTV = false;
	bool m_hasDSV = false;
	bool m_hasReadOnlyDSV = false;
	bool m_hasSRV = false;
	bool m_hasUAV = false;
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

class icpD3D12BindingSet : public icpRHIBindingSet
{
public:
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart{};
};

class icpD3D12CommandList : public icpRHICommandList
{
public:
	icpD3D12CommandList(icpQueueType queueType, ID3D12GraphicsCommandList* commandList)
		: m_queueType(queueType)
		, m_commandList(commandList)
	{
	}

	icpQueueType GetQueueType() const override { return m_queueType; }
	ID3D12GraphicsCommandList* GetNative() const { return m_commandList; }

private:
	icpQueueType m_queueType = icpQueueType::GRAPHICS;
	ID3D12GraphicsCommandList* m_commandList = nullptr;
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
	std::shared_ptr<icpRHIPipeline> CreateComputePipeline(
		const icpComputePipelineDesc& desc) override;
	std::shared_ptr<icpRHIBindingSet> CreateBindingSet(
		const icpRHIBindingSetDesc& desc) override;

	bool SupportsAsyncCompute() const override;
	std::shared_ptr<icpRHICommandList> GetGraphicsCommandList() override;
	std::shared_ptr<icpRHICommandList> BeginAsyncCompute() override;
	uint64_t EndAsyncCompute(std::shared_ptr<icpRHICommandList> commandList) override;
	void SubmitGraphicsWorkBeforeAsyncCompute() override;
	void WaitForAsyncCompute(uint64_t fenceValue) override;

	void PrepareCommandList(std::shared_ptr<icpRHICommandList> commandList) override;
	void TransitionTexture(
		std::shared_ptr<icpRHICommandList> commandList,
		std::shared_ptr<icpRHITexture> texture,
		icpResourceState newState) override;
	void TransitionBackBuffer(
		std::shared_ptr<icpRHICommandList> commandList,
		icpResourceState newState) override;
	void SetViewportAndScissor(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t width,
		uint32_t height) override;
	void SetRenderTargets(
		std::shared_ptr<icpRHICommandList> commandList,
		const std::vector<std::shared_ptr<icpRHITexture>>& colorTargets,
		std::shared_ptr<icpRHITexture> depthTarget,
		icpRHIDepthAccess depthAccess,
		bool clearColor,
		bool clearDepth) override;
	void SetBackBufferRenderTarget(
		std::shared_ptr<icpRHICommandList> commandList,
		bool clearColor) override;
	void SetBackBufferRenderTarget(
		std::shared_ptr<icpRHICommandList> commandList,
		std::shared_ptr<icpRHITexture> depthTarget,
		icpRHIDepthAccess depthAccess,
		bool clearColor) override;
	void BindGraphicsPipeline(
		std::shared_ptr<icpRHICommandList> commandList,
		std::shared_ptr<icpRHIPipeline> pipeline) override;
	void BindComputePipeline(
		std::shared_ptr<icpRHICommandList> commandList,
		std::shared_ptr<icpRHIPipeline> pipeline) override;
	void BindGraphicsConstantBuffer(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t bindingIndex,
		std::shared_ptr<icpRHIBuffer> buffer) override;
	void BindComputeConstantBuffer(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t bindingIndex,
		std::shared_ptr<icpRHIBuffer> buffer) override;
	void BindGraphicsBindingSet(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t bindingIndex,
		std::shared_ptr<icpRHIBindingSet> bindingSet) override;
	void BindComputeBindingSet(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t bindingIndex,
		std::shared_ptr<icpRHIBindingSet> bindingSet) override;
	void SetGraphicsConstant(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t bindingIndex,
		uint32_t value) override;
	void BindVertexAndIndexBuffers(
		std::shared_ptr<icpRHICommandList> commandList,
		std::shared_ptr<icpRHIBuffer> vertexBuffer,
		uint64_t vertexBufferSize,
		std::shared_ptr<icpRHIBuffer> indexBuffer,
		uint64_t indexBufferSize,
		uint32_t vertexStride) override;
	void DrawIndexed(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t indexCount) override;
	void Draw(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t vertexCount) override;
	void Dispatch(
		std::shared_ptr<icpRHICommandList> commandList,
		uint32_t groupCountX,
		uint32_t groupCountY,
		uint32_t groupCountZ) override;

	void InitializeImGui(std::shared_ptr<icpWindowSystem> windowSystem) override;
	void ShutdownImGui() override;
	void BeginImGuiFrame() override;
	void RenderImGuiDrawData(std::shared_ptr<icpRHICommandList> commandList) override;

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
	void TransitionResource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
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
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_computeQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[MAX_FRAMES_IN_FLIGHT];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_graphicsContinuationAllocators[MAX_FRAMES_IN_FLIGHT];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_computeCommandAllocators[MAX_FRAMES_IN_FLIGHT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_computeCommandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_asyncFence;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_backBuffers[MAX_FRAMES_IN_FLIGHT];
	D3D12_CPU_DESCRIPTOR_HANDLE m_backBufferRTVs[MAX_FRAMES_IN_FLIGHT]{};

	HANDLE m_fenceEvent = nullptr;
	uint64_t m_fenceValues[MAX_FRAMES_IN_FLIGHT]{};
	uint64_t m_asyncFenceValue = 0;
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
