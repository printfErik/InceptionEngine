#include "icpD3D12GPUDevice.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_glfw.h>
#include <imgui.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <utility>

INCEPTION_BEGIN_NAMESPACE

namespace
{
static void ThrowIfFailed(HRESULT hr, const char* message)
{
	if (FAILED(hr))
	{
		throw std::runtime_error(message);
	}
}

static uint64_t Align256(uint64_t value)
{
	return (value + 255ull) & ~255ull;
}

static std::vector<uint8_t> ReadBinaryFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file)
	{
		throw std::runtime_error("failed to open shader file: " + path.string());
	}

	const auto size = static_cast<size_t>(file.tellg());
	std::vector<uint8_t> data(size);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(data.data()), size);
	return data;
}

static ID3D12GraphicsCommandList* NativeCommandList(const std::shared_ptr<icpRHICommandList>& commandList)
{
	return static_cast<icpD3D12CommandList*>(commandList.get())->GetNative();
}

static icpD3D12Texture* D3D12Texture(const std::shared_ptr<icpRHITexture>& texture)
{
	return static_cast<icpD3D12Texture*>(texture.get());
}

static icpD3D12Buffer* D3D12Buffer(const std::shared_ptr<icpRHIBuffer>& buffer)
{
	return static_cast<icpD3D12Buffer*>(buffer.get());
}

static icpD3D12Pipeline* D3D12Pipeline(const std::shared_ptr<icpRHIPipeline>& pipeline)
{
	return static_cast<icpD3D12Pipeline*>(pipeline.get());
}

static icpD3D12BindingSet* D3D12BindingSet(const std::shared_ptr<icpRHIBindingSet>& bindingSet)
{
	return static_cast<icpD3D12BindingSet*>(bindingSet.get());
}
}

icpD3D12Buffer::icpD3D12Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource, uint64_t size)
	: m_resource(std::move(resource))
	, m_size(size)
{
}

void* icpD3D12Buffer::Map()
{
	void* data = nullptr;
	D3D12_RANGE readRange{ 0, 0 };
	ThrowIfFailed(m_resource->Map(0, &readRange, &data), "failed to map d3d12 buffer");
	return data;
}

void icpD3D12Buffer::Unmap()
{
	m_resource->Unmap(0, nullptr);
}

uint64_t icpD3D12Buffer::GetGPUAddress() const
{
	return m_resource->GetGPUVirtualAddress();
}

uint64_t icpD3D12Buffer::GetSize() const
{
	return m_size;
}

icpD3D12GPUDevice::~icpD3D12GPUDevice()
{
	if (m_device)
	{
		WaitIdle();
	}
	ReleaseSwapchainResources();
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

bool icpD3D12GPUDevice::Initialize(std::shared_ptr<icpWindowSystem> windowSystem)
{
	m_windowSystem = windowSystem;
	CreateFactory();
	CreateDevice();
	CreateCommandObjects();
	CreateDescriptorHeaps();
	CreateSwapchain();
	CreateBackBufferRTVs();
	CreateFenceObjects();
	return true;
}

void icpD3D12GPUDevice::CreateFactory()
{
#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif
	UINT flags = 0;
#if defined(_DEBUG)
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)), "failed to create dxgi factory");
}

void icpD3D12GPUDevice::CreateDevice()
{
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &m_adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc{};
		m_adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}
		if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
		{
			return;
		}
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
	ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)), "failed to enumerate WARP adapter");
	ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)), "failed to create d3d12 device");
}

void icpD3D12GPUDevice::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_graphicsQueue)), "failed to create d3d12 queue");

	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_computeQueue)), "failed to create d3d12 compute queue");

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])), "failed to create command allocator");
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_graphicsContinuationAllocators[i])), "failed to create continuation command allocator");
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_computeCommandAllocators[i])), "failed to create compute command allocator");
	}

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)), "failed to create command list");
	ThrowIfFailed(m_commandList->Close(), "failed to close initial command list");

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_computeCommandList)), "failed to create compute command list");
	ThrowIfFailed(m_computeCommandList->Close(), "failed to close initial compute command list");
}

void icpD3D12GPUDevice::CreateDescriptorHeaps()
{
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = 128;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)), "failed to create rtv heap");

	D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
	dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDesc.NumDescriptors = 64;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsvHeap)), "failed to create dsv heap");

	D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
	srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDesc.NumDescriptors = 4096;
	srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_srvHeap)), "failed to create srv heap");
}

void icpD3D12GPUDevice::CreateSwapchain()
{
	int width = 1;
	int height = 1;
	glfwGetFramebufferSize(m_windowSystem->getWindow(), &width, &height);
	m_backBufferWidth = static_cast<uint32_t>((std::max)(width, 1));
	m_backBufferHeight = static_cast<uint32_t>((std::max)(height, 1));

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
	swapchainDesc.BufferCount = MAX_FRAMES_IN_FLIGHT;
	swapchainDesc.Width = m_backBufferWidth;
	swapchainDesc.Height = m_backBufferHeight;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;
	HWND hwnd = glfwGetWin32Window(m_windowSystem->getWindow());
	ThrowIfFailed(m_factory->CreateSwapChainForHwnd(m_graphicsQueue.Get(), hwnd, &swapchainDesc, nullptr, nullptr, &swapchain1), "failed to create swapchain");
	ThrowIfFailed(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER), "failed to set window association");
	ThrowIfFailed(swapchain1.As(&m_swapchain), "failed to query IDXGISwapChain3");
	m_currentFrame = m_swapchain->GetCurrentBackBufferIndex();
}

void icpD3D12GPUDevice::CreateBackBufferRTVs()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		ThrowIfFailed(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])), "failed to get swapchain backbuffer");
		auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T>(i) * m_rtvDescriptorSize;
		m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtv);
		m_backBufferRTVs[i] = rtv;
	}
}

void icpD3D12GPUDevice::CreateFenceObjects()
{
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "failed to create fence");
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_asyncFence)), "failed to create async compute fence");
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!m_fenceEvent)
	{
		throw std::runtime_error("failed to create fence event");
	}
	for (auto& value : m_fenceValues)
	{
		value = 1;
	}
}

void icpD3D12GPUDevice::BeginFrame()
{
	WaitForFrame(m_currentFrame);
	ThrowIfFailed(m_commandAllocators[m_currentFrame]->Reset(), "failed to reset command allocator");
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_currentFrame].Get(), nullptr), "failed to reset command list");
}

void icpD3D12GPUDevice::EndFrame()
{
	ThrowIfFailed(m_commandList->Close(), "failed to close command list");
	ID3D12CommandList* lists[] = { m_commandList.Get() };
	m_graphicsQueue->ExecuteCommandLists(1, lists);
	ThrowIfFailed(m_swapchain->Present(1, 0), "failed to present swapchain");
	MoveToNextFrame();
}

void icpD3D12GPUDevice::MoveToNextFrame()
{
	const uint64_t currentFenceValue = m_fenceValues[m_currentFrame];
	ThrowIfFailed(m_graphicsQueue->Signal(m_fence.Get(), currentFenceValue), "failed to signal frame fence");
	m_currentFrame = m_swapchain->GetCurrentBackBufferIndex();
	if (m_fence->GetCompletedValue() < m_fenceValues[m_currentFrame])
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_currentFrame], m_fenceEvent), "failed to set fence event");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_fenceValues[m_currentFrame] = currentFenceValue + 1;
}

void icpD3D12GPUDevice::WaitForFrame(uint32_t frameIndex)
{
	if (m_fence->GetCompletedValue() < m_fenceValues[frameIndex] - 1)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[frameIndex] - 1, m_fenceEvent), "failed to wait frame fence");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void icpD3D12GPUDevice::WaitIdle()
{
	const uint64_t fenceValue = m_fenceValues[m_currentFrame]++;
	ThrowIfFailed(m_graphicsQueue->Signal(m_fence.Get(), fenceValue), "failed to signal idle fence");
	ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent), "failed to set idle fence");
	WaitForSingleObject(m_fenceEvent, INFINITE);

	if (m_computeQueue && m_asyncFence)
	{
		const uint64_t asyncFenceValue = ++m_asyncFenceValue;
		ThrowIfFailed(m_computeQueue->Signal(m_asyncFence.Get(), asyncFenceValue), "failed to signal compute idle fence");
		ThrowIfFailed(m_asyncFence->SetEventOnCompletion(asyncFenceValue, m_fenceEvent), "failed to set compute idle fence");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void icpD3D12GPUDevice::ReleaseSwapchainResources()
{
	for (auto& backBuffer : m_backBuffers)
	{
		backBuffer.Reset();
	}
}

void icpD3D12GPUDevice::ResizeSwapchain()
{
	WaitIdle();
	ReleaseSwapchainResources();

	int width = 1;
	int height = 1;
	glfwGetFramebufferSize(m_windowSystem->getWindow(), &width, &height);
	m_backBufferWidth = static_cast<uint32_t>((std::max)(width, 1));
	m_backBufferHeight = static_cast<uint32_t>((std::max)(height, 1));

	ThrowIfFailed(m_swapchain->ResizeBuffers(MAX_FRAMES_IN_FLIGHT, m_backBufferWidth, m_backBufferHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0), "failed to resize swapchain");
	m_currentFrame = m_swapchain->GetCurrentBackBufferIndex();
	CreateBackBufferRTVs();
}

std::shared_ptr<icpRHIBuffer> icpD3D12GPUDevice::CreateBuffer(const icpRHIBufferDesc& desc, const void* initialData)
{
	const uint64_t size = HasUsage(desc.usage, icpBufferUsage::UNIFORM) ? Align256(desc.size) : desc.size;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)), "failed to create d3d12 buffer");

	if (initialData && desc.size > 0)
	{
		void* data = nullptr;
		D3D12_RANGE readRange{ 0, 0 };
		ThrowIfFailed(resource->Map(0, &readRange, &data), "failed to map initial buffer");
		memcpy(data, initialData, static_cast<size_t>(desc.size));
		resource->Unmap(0, nullptr);
	}

	return std::make_shared<icpD3D12Buffer>(resource, size);
}

std::shared_ptr<icpRHITexture> icpD3D12GPUDevice::CreateTexture(const icpRHITextureDesc& desc, const void* initialData, size_t initialDataSize)
{
	auto texture = std::make_shared<icpD3D12Texture>();
	texture->m_format = desc.format;
	texture->m_state = desc.initialState;
	texture->m_width = desc.width;
	texture->m_height = desc.height;

	DXGI_FORMAT resourceFormat = ToDXGIFormat(desc.format);
	if (desc.format == icpFormat::D32_FLOAT)
	{
		resourceFormat = DXGI_FORMAT_R32_TYPELESS;
	}

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = desc.width;
	resourceDesc.Height = desc.height;
	resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.arraySize);
	resourceDesc.MipLevels = static_cast<UINT16>(desc.mipLevels);
	resourceDesc.Format = resourceFormat;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	if (HasUsage(desc.usage, icpTextureUsage::RENDER_TARGET))
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if (HasUsage(desc.usage, icpTextureUsage::DEPTH_STENCIL))
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	if (HasUsage(desc.usage, icpTextureUsage::STORAGE))
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue{};
	D3D12_CLEAR_VALUE* clearValuePtr = nullptr;
	if (HasUsage(desc.usage, icpTextureUsage::RENDER_TARGET))
	{
		clearValue.Format = ToDXGIFormat(desc.format);
		clearValue.Color[0] = 0.f;
		clearValue.Color[1] = 0.f;
		clearValue.Color[2] = 0.f;
		clearValue.Color[3] = 1.f;
		clearValuePtr = &clearValue;
	}
	else if (HasUsage(desc.usage, icpTextureUsage::DEPTH_STENCIL))
	{
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.f;
		clearValue.DepthStencil.Stencil = 0;
		clearValuePtr = &clearValue;
	}

	const auto initialState = initialData ? D3D12_RESOURCE_STATE_COPY_DEST : ToD3D12State(desc.initialState);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		clearValuePtr,
		IID_PPV_ARGS(&texture->m_resource)), "failed to create d3d12 texture");

	if (HasUsage(desc.usage, icpTextureUsage::RENDER_TARGET))
	{
		texture->m_rtv = AllocateRTV();
		m_device->CreateRenderTargetView(texture->m_resource.Get(), nullptr, texture->m_rtv);
		texture->m_hasRTV = true;
	}

	if (HasUsage(desc.usage, icpTextureUsage::DEPTH_STENCIL))
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		texture->m_dsv = AllocateDSV();
		m_device->CreateDepthStencilView(texture->m_resource.Get(), &dsvDesc, texture->m_dsv);
		texture->m_hasDSV = true;

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		texture->m_readOnlyDsv = AllocateDSV();
		m_device->CreateDepthStencilView(texture->m_resource.Get(), &dsvDesc, texture->m_readOnlyDsv);
		texture->m_hasReadOnlyDSV = true;
	}

	if (HasUsage(desc.usage, icpTextureUsage::SAMPLED))
	{
		auto [cpu, gpu] = AllocateSRV();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = desc.format == icpFormat::D32_FLOAT ? DXGI_FORMAT_R32_FLOAT : ToDXGIFormat(desc.format, true);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.mipLevels;
		m_device->CreateShaderResourceView(texture->m_resource.Get(), &srvDesc, cpu);
		texture->m_srvCpu = cpu;
		texture->m_srvGpu = gpu;
		texture->m_hasSRV = true;
	}

	if (HasUsage(desc.usage, icpTextureUsage::STORAGE))
	{
		auto [cpu, gpu] = AllocateSRV();
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = ToDXGIFormat(desc.format, true);
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(texture->m_resource.Get(), nullptr, &uavDesc, cpu);
		texture->m_uavCpu = cpu;
		texture->m_uavGpu = gpu;
		texture->m_hasUAV = true;
	}

	if (initialData && initialDataSize > 0)
	{
		D3D12_RESOURCE_DESC textureDesc = texture->m_resource->GetDesc();
		UINT64 uploadBufferSize = 0;
		m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

		D3D12_HEAP_PROPERTIES uploadHeap{};
		uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC uploadDesc{};
		uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadDesc.Width = uploadBufferSize;
		uploadDesc.Height = 1;
		uploadDesc.DepthOrArraySize = 1;
		uploadDesc.MipLevels = 1;
		uploadDesc.SampleDesc.Count = 1;
		uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
		ThrowIfFailed(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)), "failed to create texture upload buffer");

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT rows = 0;
		UINT64 rowSize = 0;
		UINT64 totalBytes = 0;
		m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &rows, &rowSize, &totalBytes);

		void* mapped = nullptr;
		D3D12_RANGE readRange{ 0, 0 };
		ThrowIfFailed(uploadBuffer->Map(0, &readRange, &mapped), "failed to map texture upload");
		const auto* src = reinterpret_cast<const uint8_t*>(initialData);
		auto* dst = reinterpret_cast<uint8_t*>(mapped) + footprint.Offset;
		const size_t srcRowPitch = static_cast<size_t>(desc.width) * 4u;
		for (UINT row = 0; row < rows; ++row)
		{
			memcpy(dst + row * footprint.Footprint.RowPitch, src + row * srcRowPitch, std::min(srcRowPitch, static_cast<size_t>(rowSize)));
		}
		uploadBuffer->Unmap(0, nullptr);

		ExecuteImmediate([&](ID3D12GraphicsCommandList* cmd)
		{
			D3D12_TEXTURE_COPY_LOCATION dstLoc{};
			dstLoc.pResource = texture->m_resource.Get();
			dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLoc.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION srcLoc{};
			srcLoc.pResource = uploadBuffer.Get();
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLoc.PlacedFootprint = footprint;
			cmd->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = texture->m_resource.Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = ToD3D12State(desc.initialState);
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmd->ResourceBarrier(1, &barrier);
		});
		texture->m_state = desc.initialState;
	}

	return texture;
}

std::shared_ptr<icpRHISampler> icpD3D12GPUDevice::CreateSampler()
{
	return std::make_shared<icpD3D12Sampler>();
}

std::shared_ptr<icpRHIPipeline> icpD3D12GPUDevice::CreateGraphicsPipeline(const icpGraphicsPipelineDesc& desc)
{
	auto pipeline = std::make_shared<icpD3D12Pipeline>();
	pipeline->m_kind = desc.kind;

	D3D12_DESCRIPTOR_RANGE ranges[2]{};
	std::vector<D3D12_ROOT_PARAMETER> params;

	if (desc.kind == icpPipelineKind::GBUFFER || desc.kind == icpPipelineKind::FORWARD_TRANSLUCENT)
	{
		params.resize(4);
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[0].Descriptor.ShaderRegister = 0;
		params[0].Descriptor.RegisterSpace = 0;
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[1].Descriptor.ShaderRegister = 0;
		params[1].Descriptor.RegisterSpace = 1;
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 7;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 1;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[2].DescriptorTable.NumDescriptorRanges = 1;
		params[2].DescriptorTable.pDescriptorRanges = &ranges[0];
		params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[3].Descriptor.ShaderRegister = 0;
		params[3].Descriptor.RegisterSpace = 2;
		params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}
	else if (desc.kind == icpPipelineKind::DEFERRED_COMPOSITE)
	{
		params.resize(3);
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 9;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[0].DescriptorTable.NumDescriptorRanges = 1;
		params[0].DescriptorTable.pDescriptorRanges = &ranges[0];
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[1].Descriptor.ShaderRegister = 0;
		params[1].Descriptor.RegisterSpace = 2;
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[2].Descriptor.ShaderRegister = 0;
		params[2].Descriptor.RegisterSpace = 3;
		params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	else if (desc.kind == icpPipelineKind::CSM)
	{
		params.resize(3);
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[0].Descriptor.ShaderRegister = 0;
		params[0].Descriptor.RegisterSpace = 0;
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[1].Descriptor.ShaderRegister = 0;
		params[1].Descriptor.RegisterSpace = 3;
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		params[2].Constants.ShaderRegister = 1;
		params[2].Constants.RegisterSpace = 3;
		params[2].Constants.Num32BitValues = 1;
		params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}
	else
	{
		throw std::runtime_error("unsupported graphics pipeline kind");
	}

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.NumParameters = static_cast<UINT>(params.size());
	rootDesc.pParameters = params.data();
	rootDesc.NumStaticSamplers = 1;
	rootDesc.pStaticSamplers = &sampler;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error), "failed to serialize root signature");
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pipeline->m_rootSignature)), "failed to create root signature");

	const auto vs = ReadBinaryFile(desc.vertexShader);
	const auto ps = ReadBinaryFile(desc.pixelShader);

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	for (const auto& attr : desc.vertexAttributes)
	{
		const char* semantic = "POSITION";
		if (attr.location == 1) semantic = "COLOR";
		else if (attr.location == 2) semantic = "NORMAL";
		else if (attr.location == 3) semantic = "TEXCOORD";

		inputElements.push_back({
			semantic,
			0,
			ToDXGIFormat(attr.format),
			0,
			attr.offset,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		});
	}

	std::vector<DXGI_FORMAT> rtvFormats;
	for (auto format : desc.renderTargetFormats)
	{
		rtvFormats.push_back(ToDXGIFormat(format));
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = pipeline->m_rootSignature.Get();
	psoDesc.VS = { vs.data(), vs.size() };
	psoDesc.PS = { ps.data(), ps.size() };
	psoDesc.InputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = static_cast<UINT>(rtvFormats.size());
	for (UINT i = 0; i < psoDesc.NumRenderTargets; ++i)
	{
		psoDesc.RTVFormats[i] = rtvFormats[i];
	}
	psoDesc.DSVFormat = desc.depthFormat == icpFormat::UNKNOWN ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = desc.cullMode == icpCullMode::NONE ? D3D12_CULL_MODE_NONE : desc.cullMode == icpCullMode::FRONT ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = desc.kind == icpPipelineKind::CSM ? 1000 : D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = desc.kind == icpPipelineKind::CSM ? 1.5f : D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	psoDesc.DepthStencilState.DepthEnable = desc.depthTestEnable;
	psoDesc.DepthStencilState.DepthWriteMask = desc.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = desc.depthCompare == icpCompareOp::ALWAYS ? D3D12_COMPARISON_FUNC_ALWAYS : D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultBlend = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};
	for (auto& rtBlend : psoDesc.BlendState.RenderTarget)
	{
		rtBlend = defaultBlend;
	}
	if (desc.blendMode == icpBlendMode::TRANSLUCENT)
	{
		auto& rtBlend = psoDesc.BlendState.RenderTarget[0];
		rtBlend.BlendEnable = TRUE;
		rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
		rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline->m_pipelineState)), "failed to create graphics pipeline");
	return pipeline;
}

std::shared_ptr<icpRHIPipeline> icpD3D12GPUDevice::CreateComputePipeline(const icpComputePipelineDesc& desc)
{
	auto pipeline = std::make_shared<icpD3D12Pipeline>();
	pipeline->m_kind = desc.kind;

	D3D12_DESCRIPTOR_RANGE ranges[2]{};
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 2;
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[1].NumDescriptors = 1;
	ranges[1].BaseShaderRegister = 0;
	ranges[1].RegisterSpace = 0;
	ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER params[3]{};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[0].DescriptorTable.NumDescriptorRanges = 1;
	params[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].DescriptorTable.NumDescriptorRanges = 1;
	params[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[2].Descriptor.ShaderRegister = 0;
	params[2].Descriptor.RegisterSpace = 2;
	params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.NumParameters = static_cast<UINT>(sizeof(params) / sizeof(params[0]));
	rootDesc.pParameters = params;
	rootDesc.NumStaticSamplers = 1;
	rootDesc.pStaticSamplers = &sampler;

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error), "failed to serialize compute root signature");
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pipeline->m_rootSignature)), "failed to create compute root signature");

	const auto cs = ReadBinaryFile(desc.computeShader);
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = pipeline->m_rootSignature.Get();
	psoDesc.CS = { cs.data(), cs.size() };
	ThrowIfFailed(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipeline->m_pipelineState)), "failed to create compute pipeline");
	return pipeline;
}

std::shared_ptr<icpRHIBindingSet> icpD3D12GPUDevice::CreateBindingSet(const icpRHIBindingSetDesc& desc)
{
	auto bindingSet = std::make_shared<icpD3D12BindingSet>();
	if (desc.resources.empty())
	{
		return bindingSet;
	}

	auto [cpuStart, gpuStart] = AllocateSRV();
	bindingSet->m_gpuStart = gpuStart;
	auto cpu = cpuStart;
	for (size_t i = 0; i < desc.resources.size(); ++i)
	{
		if (i > 0)
		{
			AllocateSRV();
			cpu.ptr += m_srvDescriptorSize;
		}

		auto* texture = D3D12Texture(desc.resources[i].texture);
		const auto source = desc.resources[i].viewType == icpRHIResourceViewType::UAV ? texture->m_uavCpu : texture->m_srvCpu;
		m_device->CopyDescriptorsSimple(1, cpu, source, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	return bindingSet;
}

uint32_t icpD3D12GPUDevice::GetCurrentFrameIndex() const { return m_currentFrame; }
uint32_t icpD3D12GPUDevice::GetBackBufferWidth() const { return m_backBufferWidth; }
uint32_t icpD3D12GPUDevice::GetBackBufferHeight() const { return m_backBufferHeight; }
ID3D12Device* icpD3D12GPUDevice::GetDevice() const { return m_device.Get(); }
ID3D12GraphicsCommandList* icpD3D12GPUDevice::GetCommandList() const { return m_commandList.Get(); }
ID3D12CommandQueue* icpD3D12GPUDevice::GetGraphicsQueue() const { return m_graphicsQueue.Get(); }
IDXGISwapChain3* icpD3D12GPUDevice::GetSwapchain() const { return m_swapchain.Get(); }
D3D12_CPU_DESCRIPTOR_HANDLE icpD3D12GPUDevice::GetCurrentBackBufferRTV() const { return m_backBufferRTVs[m_currentFrame]; }
ID3D12Resource* icpD3D12GPUDevice::GetCurrentBackBuffer() const { return m_backBuffers[m_currentFrame].Get(); }
ID3D12DescriptorHeap* icpD3D12GPUDevice::GetShaderVisibleHeap() const { return m_srvHeap.Get(); }

D3D12_CPU_DESCRIPTOR_HANDLE icpD3D12GPUDevice::AllocateRTV()
{
	auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(m_nextRTV++) * m_rtvDescriptorSize;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE icpD3D12GPUDevice::AllocateDSV()
{
	auto handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(m_nextDSV++) * m_dsvDescriptorSize;
	return handle;
}

std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> icpD3D12GPUDevice::AllocateSRV()
{
	auto cpu = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	auto gpu = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	cpu.ptr += static_cast<SIZE_T>(m_nextSRV) * m_srvDescriptorSize;
	gpu.ptr += static_cast<UINT64>(m_nextSRV) * m_srvDescriptorSize;
	++m_nextSRV;
	return { cpu, gpu };
}

bool icpD3D12GPUDevice::SupportsAsyncCompute() const
{
	return m_computeQueue.Get() != nullptr;
}

std::shared_ptr<icpRHICommandList> icpD3D12GPUDevice::GetGraphicsCommandList()
{
	return std::make_shared<icpD3D12CommandList>(icpQueueType::GRAPHICS, m_commandList.Get());
}

void icpD3D12GPUDevice::SubmitGraphicsWorkBeforeAsyncCompute()
{
	ThrowIfFailed(m_commandList->Close(), "failed to close graphics list before async compute");
	ID3D12CommandList* lists[] = { m_commandList.Get() };
	m_graphicsQueue->ExecuteCommandLists(1, lists);

	const uint64_t fenceValue = ++m_asyncFenceValue;
	ThrowIfFailed(m_graphicsQueue->Signal(m_asyncFence.Get(), fenceValue), "failed to signal graphics-to-compute fence");
	ThrowIfFailed(m_computeQueue->Wait(m_asyncFence.Get(), fenceValue), "failed to make compute queue wait for graphics");

	ThrowIfFailed(m_graphicsContinuationAllocators[m_currentFrame]->Reset(), "failed to reset graphics continuation allocator");
	ThrowIfFailed(m_commandList->Reset(m_graphicsContinuationAllocators[m_currentFrame].Get(), nullptr), "failed to reset graphics continuation command list");
}

std::shared_ptr<icpRHICommandList> icpD3D12GPUDevice::BeginAsyncCompute()
{
	ThrowIfFailed(m_computeCommandAllocators[m_currentFrame]->Reset(), "failed to reset compute command allocator");
	ThrowIfFailed(m_computeCommandList->Reset(m_computeCommandAllocators[m_currentFrame].Get(), nullptr), "failed to reset compute command list");
	ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
	m_computeCommandList->SetDescriptorHeaps(1, heaps);
	return std::make_shared<icpD3D12CommandList>(icpQueueType::COMPUTE, m_computeCommandList.Get());
}

uint64_t icpD3D12GPUDevice::EndAsyncCompute(std::shared_ptr<icpRHICommandList> commandList)
{
	(void)commandList;
	ThrowIfFailed(m_computeCommandList->Close(), "failed to close compute command list");
	ID3D12CommandList* lists[] = { m_computeCommandList.Get() };
	m_computeQueue->ExecuteCommandLists(1, lists);
	const uint64_t fenceValue = ++m_asyncFenceValue;
	ThrowIfFailed(m_computeQueue->Signal(m_asyncFence.Get(), fenceValue), "failed to signal async compute fence");
	return fenceValue;
}

void icpD3D12GPUDevice::WaitForAsyncCompute(uint64_t fenceValue)
{
	if (fenceValue == 0)
	{
		return;
	}
	ThrowIfFailed(m_graphicsQueue->Wait(m_asyncFence.Get(), fenceValue), "failed to make graphics queue wait for async compute");
}

void icpD3D12GPUDevice::PrepareCommandList(std::shared_ptr<icpRHICommandList> commandList)
{
	ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
	NativeCommandList(commandList)->SetDescriptorHeaps(1, heaps);
}

void icpD3D12GPUDevice::TransitionTexture(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHITexture> texture,
	icpResourceState newState)
{
	auto* d3dTexture = D3D12Texture(texture);
	TransitionResource(NativeCommandList(commandList), d3dTexture->m_resource.Get(), ToD3D12State(d3dTexture->m_state), ToD3D12State(newState));
	d3dTexture->m_state = newState;
}

void icpD3D12GPUDevice::TransitionBackBuffer(
	std::shared_ptr<icpRHICommandList> commandList,
	icpResourceState newState)
{
	const auto before = newState == icpResourceState::RENDER_TARGET ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_RENDER_TARGET;
	TransitionResource(NativeCommandList(commandList), GetCurrentBackBuffer(), before, ToD3D12State(newState));
}

void icpD3D12GPUDevice::SetViewportAndScissor(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t width,
	uint32_t height)
{
	D3D12_VIEWPORT viewport{ 0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	auto* cmd = NativeCommandList(commandList);
	cmd->RSSetViewports(1, &viewport);
	cmd->RSSetScissorRects(1, &scissor);
}

void icpD3D12GPUDevice::SetRenderTargets(
	std::shared_ptr<icpRHICommandList> commandList,
	const std::vector<std::shared_ptr<icpRHITexture>>& colorTargets,
	std::shared_ptr<icpRHITexture> depthTarget,
	icpRHIDepthAccess depthAccess,
	bool clearColor,
	bool clearDepth)
{
	auto* cmd = NativeCommandList(commandList);
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
	rtvs.reserve(colorTargets.size());
	for (const auto& target : colorTargets)
	{
		rtvs.push_back(D3D12Texture(target)->m_rtv);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
	D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;
	if (depthTarget)
	{
		auto* depth = D3D12Texture(depthTarget);
		dsv = depthAccess == icpRHIDepthAccess::READ ? depth->m_readOnlyDsv : depth->m_dsv;
		dsvPtr = &dsv;
	}

	cmd->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.empty() ? nullptr : rtvs.data(), FALSE, dsvPtr);

	if (clearColor)
	{
		const float clear[4] = { 0.f, 0.f, 0.f, 1.f };
		for (const auto& rtv : rtvs)
		{
			cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
		}
	}
	if (clearDepth && dsvPtr)
	{
		cmd->ClearDepthStencilView(*dsvPtr, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	}
}

void icpD3D12GPUDevice::SetBackBufferRenderTarget(
	std::shared_ptr<icpRHICommandList> commandList,
	bool clearColor)
{
	auto* cmd = NativeCommandList(commandList);
	auto rtv = GetCurrentBackBufferRTV();
	cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	if (clearColor)
	{
		const float clear[4] = { 0.f, 0.f, 0.f, 1.f };
		cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
	}
}

void icpD3D12GPUDevice::SetBackBufferRenderTarget(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHITexture> depthTarget,
	icpRHIDepthAccess depthAccess,
	bool clearColor)
{
	auto* cmd = NativeCommandList(commandList);
	auto rtv = GetCurrentBackBufferRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
	D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;
	if (depthTarget)
	{
		auto* depth = D3D12Texture(depthTarget);
		dsv = depthAccess == icpRHIDepthAccess::READ ? depth->m_readOnlyDsv : depth->m_dsv;
		dsvPtr = &dsv;
	}
	cmd->OMSetRenderTargets(1, &rtv, FALSE, dsvPtr);
	if (clearColor)
	{
		const float clear[4] = { 0.f, 0.f, 0.f, 1.f };
		cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
	}
}

void icpD3D12GPUDevice::BindGraphicsPipeline(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHIPipeline> pipeline)
{
	auto* cmd = NativeCommandList(commandList);
	auto* d3dPipeline = D3D12Pipeline(pipeline);
	cmd->SetPipelineState(d3dPipeline->m_pipelineState.Get());
	cmd->SetGraphicsRootSignature(d3dPipeline->m_rootSignature.Get());
}

void icpD3D12GPUDevice::BindComputePipeline(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHIPipeline> pipeline)
{
	auto* cmd = NativeCommandList(commandList);
	auto* d3dPipeline = D3D12Pipeline(pipeline);
	cmd->SetPipelineState(d3dPipeline->m_pipelineState.Get());
	cmd->SetComputeRootSignature(d3dPipeline->m_rootSignature.Get());
}

void icpD3D12GPUDevice::BindGraphicsConstantBuffer(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBuffer> buffer)
{
	NativeCommandList(commandList)->SetGraphicsRootConstantBufferView(bindingIndex, D3D12Buffer(buffer)->GetGPUAddress());
}

void icpD3D12GPUDevice::BindComputeConstantBuffer(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBuffer> buffer)
{
	NativeCommandList(commandList)->SetComputeRootConstantBufferView(bindingIndex, D3D12Buffer(buffer)->GetGPUAddress());
}

void icpD3D12GPUDevice::BindGraphicsBindingSet(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBindingSet> bindingSet)
{
	NativeCommandList(commandList)->SetGraphicsRootDescriptorTable(bindingIndex, D3D12BindingSet(bindingSet)->m_gpuStart);
}

void icpD3D12GPUDevice::BindComputeBindingSet(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBindingSet> bindingSet)
{
	NativeCommandList(commandList)->SetComputeRootDescriptorTable(bindingIndex, D3D12BindingSet(bindingSet)->m_gpuStart);
}

void icpD3D12GPUDevice::SetGraphicsConstant(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	uint32_t value)
{
	NativeCommandList(commandList)->SetGraphicsRoot32BitConstant(bindingIndex, value, 0);
}

void icpD3D12GPUDevice::BindVertexAndIndexBuffers(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHIBuffer> vertexBuffer,
	uint64_t vertexBufferSize,
	std::shared_ptr<icpRHIBuffer> indexBuffer,
	uint64_t indexBufferSize,
	uint32_t vertexStride)
{
	auto* cmd = NativeCommandList(commandList);
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = D3D12Buffer(vertexBuffer)->GetGPUAddress();
	vbv.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vbv.StrideInBytes = vertexStride;
	D3D12_INDEX_BUFFER_VIEW ibv{};
	ibv.BufferLocation = D3D12Buffer(indexBuffer)->GetGPUAddress();
	ibv.SizeInBytes = static_cast<UINT>(indexBufferSize);
	ibv.Format = DXGI_FORMAT_R32_UINT;

	cmd->IASetVertexBuffers(0, 1, &vbv);
	cmd->IASetIndexBuffer(&ibv);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void icpD3D12GPUDevice::DrawIndexed(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t indexCount)
{
	NativeCommandList(commandList)->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void icpD3D12GPUDevice::Draw(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t vertexCount)
{
	auto* cmd = NativeCommandList(commandList);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(vertexCount, 1, 0, 0);
}

void icpD3D12GPUDevice::Dispatch(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ)
{
	NativeCommandList(commandList)->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void icpD3D12GPUDevice::InitializeImGui(std::shared_ptr<icpWindowSystem> windowSystem)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForOther(windowSystem->getWindow(), true);
	auto [fontCpu, fontGpu] = AllocateSRV();
	ImGui_ImplDX12_Init(
		m_device.Get(),
		MAX_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_srvHeap.Get(),
		fontCpu,
		fontGpu);
}

void icpD3D12GPUDevice::ShutdownImGui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void icpD3D12GPUDevice::BeginImGuiFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void icpD3D12GPUDevice::RenderImGuiDrawData(std::shared_ptr<icpRHICommandList> commandList)
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), NativeCommandList(commandList));
}

D3D12_GPU_DESCRIPTOR_HANDLE icpD3D12GPUDevice::CreateTextureSRVTable(const std::vector<std::shared_ptr<icpRHITexture>>& textures)
{
	auto [cpuStart, gpuStart] = AllocateSRV();
	auto cpu = cpuStart;
	const auto increment = m_srvDescriptorSize;
	for (size_t i = 0; i < textures.size(); ++i)
	{
		if (i > 0)
		{
			AllocateSRV();
			cpu.ptr += increment;
		}

		auto* texture = static_cast<icpD3D12Texture*>(textures[i].get());
		m_device->CopyDescriptorsSimple(1, cpu, texture->m_srvCpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	return gpuStart;
}

void icpD3D12GPUDevice::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	TransitionResource(m_commandList.Get(), resource, before, after);
}

void icpD3D12GPUDevice::TransitionResource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	if (before == after)
	{
		return;
	}
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmd->ResourceBarrier(1, &barrier);
}

void icpD3D12GPUDevice::ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& record)
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "failed to create immediate allocator");
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)), "failed to create immediate list");
	record(list.Get());
	ThrowIfFailed(list->Close(), "failed to close immediate list");
	ID3D12CommandList* lists[] = { list.Get() };
	m_graphicsQueue->ExecuteCommandLists(1, lists);

	const uint64_t fenceValue = m_fenceValues[m_currentFrame]++;
	ThrowIfFailed(m_graphicsQueue->Signal(m_fence.Get(), fenceValue), "failed to signal immediate fence");
	ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent), "failed to set immediate fence");
	WaitForSingleObject(m_fenceEvent, INFINITE);
}

DXGI_FORMAT icpD3D12GPUDevice::ToDXGIFormat(icpFormat format, bool srv) const
{
	switch (format)
	{
	case icpFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case icpFormat::R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case icpFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
	case icpFormat::R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
	case icpFormat::R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case icpFormat::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
	case icpFormat::R32_FLOAT:
	case icpFormat::D32_FLOAT_SRV: return DXGI_FORMAT_R32_FLOAT;
	case icpFormat::D32_FLOAT: return srv ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_D32_FLOAT;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}

D3D12_RESOURCE_STATES icpD3D12GPUDevice::ToD3D12State(icpResourceState state) const
{
	switch (state)
	{
	case icpResourceState::COPY_DEST: return D3D12_RESOURCE_STATE_COPY_DEST;
	case icpResourceState::SHADER_RESOURCE: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case icpResourceState::NON_PIXEL_SHADER_RESOURCE: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case icpResourceState::UNORDERED_ACCESS: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case icpResourceState::RENDER_TARGET: return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case icpResourceState::DEPTH_WRITE: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case icpResourceState::DEPTH_READ: return D3D12_RESOURCE_STATE_DEPTH_READ;
	case icpResourceState::PRESENT: return D3D12_RESOURCE_STATE_PRESENT;
	default: return D3D12_RESOURCE_STATE_COMMON;
	}
}

D3D12_PRIMITIVE_TOPOLOGY icpD3D12GPUDevice::ToD3D12Topology(icpPrimitiveTopology topology) const
{
	return topology == icpPrimitiveTopology::TRIANGLE_STRIP ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

INCEPTION_END_NAMESPACE
