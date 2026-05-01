#pragma once

#include "../../core/icpMacros.h"
#include "../icpWindowSystem.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

INCEPTION_BEGIN_NAMESPACE

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

enum class icpFormat
{
	UNKNOWN = 0,
	R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB,
	R32G32_FLOAT,
	R32G32B32_FLOAT,
	R16G16B16A16_FLOAT,
	R32_FLOAT,
	R8_UNORM,
	D32_FLOAT,
	D32_FLOAT_SRV,
};

enum class icpBufferUsage : uint32_t
{
	NONE = 0,
	VERTEX = 1 << 0,
	INDEX = 1 << 1,
	UNIFORM = 1 << 2,
	UPLOAD = 1 << 3,
};

enum class icpTextureUsage : uint32_t
{
	NONE = 0,
	SAMPLED = 1 << 0,
	RENDER_TARGET = 1 << 1,
	DEPTH_STENCIL = 1 << 2,
	STORAGE = 1 << 3,
};

enum class icpResourceState
{
	UNKNOWN = 0,
	COPY_DEST,
	SHADER_RESOURCE,
	NON_PIXEL_SHADER_RESOURCE,
	UNORDERED_ACCESS,
	RENDER_TARGET,
	DEPTH_WRITE,
	DEPTH_READ,
	PRESENT,
};

enum class icpShaderStage : uint32_t
{
	VERTEX = 1 << 0,
	PIXEL = 1 << 1,
	COMPUTE = 1 << 2,
	ALL = 0xFFFFFFFF,
};

enum class icpPrimitiveTopology
{
	TRIANGLE_LIST = 0,
	TRIANGLE_STRIP,
};

enum class icpCullMode
{
	NONE = 0,
	FRONT,
	BACK,
};

enum class icpCompareOp
{
	ALWAYS = 0,
	LESS,
};

#ifdef OPAQUE
#undef OPAQUE
#endif

enum class icpBlendMode
{
	OPAQUE = 0,
	MASKED,
	TRANSLUCENT,
};

enum class icpPipelineKind
{
	GBUFFER = 0,
	DEFERRED_COMPOSITE,
	CSM,
	GTAO,
	FORWARD_TRANSLUCENT,
};

enum class icpQueueType
{
	GRAPHICS = 0,
	COMPUTE,
	COPY,
};

inline icpBufferUsage operator|(icpBufferUsage lhs, icpBufferUsage rhs)
{
	return static_cast<icpBufferUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline bool HasUsage(icpBufferUsage value, icpBufferUsage flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

inline icpTextureUsage operator|(icpTextureUsage lhs, icpTextureUsage rhs)
{
	return static_cast<icpTextureUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline bool HasUsage(icpTextureUsage value, icpTextureUsage flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

struct icpRHITextureDesc
{
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t mipLevels = 1;
	uint32_t arraySize = 1;
	icpFormat format = icpFormat::UNKNOWN;
	icpTextureUsage usage = icpTextureUsage::SAMPLED;
	icpResourceState initialState = icpResourceState::SHADER_RESOURCE;
	const char* debugName = nullptr;
};

struct icpRHIBufferDesc
{
	uint64_t size = 0;
	icpBufferUsage usage = icpBufferUsage::NONE;
	const char* debugName = nullptr;
};

struct icpVertexAttributeDesc
{
	uint32_t location = 0;
	icpFormat format = icpFormat::UNKNOWN;
	uint32_t offset = 0;
};

struct icpGraphicsPipelineDesc
{
	icpPipelineKind kind = icpPipelineKind::GBUFFER;
	std::filesystem::path vertexShader;
	std::filesystem::path pixelShader;
	std::vector<icpVertexAttributeDesc> vertexAttributes;
	uint32_t vertexStride = 0;
	std::vector<icpFormat> renderTargetFormats;
	icpFormat depthFormat = icpFormat::UNKNOWN;
	icpPrimitiveTopology topology = icpPrimitiveTopology::TRIANGLE_LIST;
	icpCullMode cullMode = icpCullMode::BACK;
	icpCompareOp depthCompare = icpCompareOp::LESS;
	bool depthTestEnable = true;
	bool depthWriteEnable = true;
	icpBlendMode blendMode = icpBlendMode::OPAQUE;
};

struct icpComputePipelineDesc
{
	icpPipelineKind kind = icpPipelineKind::GTAO;
	std::filesystem::path computeShader;
};

class icpRHIBuffer
{
public:
	virtual ~icpRHIBuffer() = default;
	virtual void* Map() = 0;
	virtual void Unmap() = 0;
	virtual uint64_t GetGPUAddress() const = 0;
	virtual uint64_t GetSize() const = 0;
};

class icpRHITexture
{
public:
	virtual ~icpRHITexture() = default;
	icpFormat m_format = icpFormat::UNKNOWN;
	icpResourceState m_state = icpResourceState::UNKNOWN;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
};

class icpRHISampler
{
public:
	virtual ~icpRHISampler() = default;
};

class icpRHIBindingLayout
{
public:
	virtual ~icpRHIBindingLayout() = default;
};

class icpRHIBindingSet
{
public:
	virtual ~icpRHIBindingSet() = default;
};

class icpRHIPipeline
{
public:
	virtual ~icpRHIPipeline() = default;
};

class icpRHICommandList
{
public:
	virtual ~icpRHICommandList() = default;
	virtual icpQueueType GetQueueType() const { return icpQueueType::GRAPHICS; }
};

class icpRHIDevice
{
public:
	virtual ~icpRHIDevice() = default;

	virtual bool Initialize(std::shared_ptr<icpWindowSystem> windowSystem) = 0;
	virtual void WaitIdle() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void ResizeSwapchain() = 0;

	virtual std::shared_ptr<icpRHIBuffer> CreateBuffer(
		const icpRHIBufferDesc& desc,
		const void* initialData = nullptr) = 0;
	virtual std::shared_ptr<icpRHITexture> CreateTexture(
		const icpRHITextureDesc& desc,
		const void* initialData = nullptr,
		size_t initialDataSize = 0) = 0;
	virtual std::shared_ptr<icpRHISampler> CreateSampler() = 0;
	virtual std::shared_ptr<icpRHIPipeline> CreateGraphicsPipeline(
		const icpGraphicsPipelineDesc& desc) = 0;
	virtual std::shared_ptr<icpRHIPipeline> CreateComputePipeline(
		const icpComputePipelineDesc& desc)
	{
		(void)desc;
		return nullptr;
	}

	virtual bool SupportsAsyncCompute() const { return false; }
	virtual std::shared_ptr<icpRHICommandList> BeginAsyncCompute() { return nullptr; }
	virtual uint64_t EndAsyncCompute(std::shared_ptr<icpRHICommandList> commandList)
	{
		(void)commandList;
		return 0;
	}
	virtual void SubmitGraphicsWorkBeforeAsyncCompute() {}
	virtual void WaitForAsyncCompute(uint64_t fenceValue) { (void)fenceValue; }

	virtual uint32_t GetCurrentFrameIndex() const = 0;
	virtual uint32_t GetBackBufferWidth() const = 0;
	virtual uint32_t GetBackBufferHeight() const = 0;

	bool m_framebufferResized = false;
};

using icpGPUDevice = icpRHIDevice;

INCEPTION_END_NAMESPACE
