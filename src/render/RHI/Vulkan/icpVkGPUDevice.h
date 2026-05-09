#pragma once
#include <optional>
#include <vector>
#include <chrono>
#include <unordered_map>

#include "../icpGPUDevice.h"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

INCEPTION_BEGIN_NAMESPACE

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;
	std::optional<uint32_t> m_transferFamily;
	std::optional<uint32_t> m_computeFamily;

	bool isComplete() const
	{
		return m_graphicsFamily.has_value() &&
			m_presentFamily.has_value() &&
			m_transferFamily.has_value() &&
			m_computeFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR        m_capabilities{};
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR>   m_presentModes;
};

class icpVkGPUDevice : public icpGPUDevice
{
public:
	icpVkGPUDevice() = default;
	~icpVkGPUDevice() override;

	bool Initialize(std::shared_ptr<icpWindowSystem> window_system) override;
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

	void cleanup();
	void CleanUpSwapChain();

	void WaitForFence(uint32_t _currentFrame);
	uint32_t AcquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result);

	void CreateSwapChain();
	void CreateSwapChainImageViews();

	void FindDepthFormat();
	void CreateDepthResources();
	void createDescriptorPools();

	void createVmaAllocator();

	VkDevice& GetLogicalDevice();
	VkPhysicalDevice& GetPhysicalDevice();

	VmaAllocator& GetVmaAllocator();
	QueueFamilyIndices& GetQueueFamilyIndices();

	VkQueue& GetTransferQueue();
	VkCommandPool& GetTransferCommandPool();

	VkCommandPool& GetGraphicsCommandPool();
	VkQueue& GetGraphicsQueue();

	VkCommandPool& GetComputeCommandPool();
	VkQueue& GetComputeQueue();

	VkQueue& GetPresentQueue();

	VkSwapchainKHR& GetSwapChain();
	VkExtent2D& GetSwapChainExtent();
	std::vector<VkImageView>& GetSwapChainImageViews();
	std::vector<VkImage>& GetSwapChainImages();
	VkFormat GetSwapChainImageFormat();

	std::vector<VkFence>& GetInFlightFences();
	std::vector<VkSemaphore>& GetImageAvailableForRenderingSemaphores();
	std::vector<VkSemaphore>& GetRenderFinishedForPresentationSemaphores();

	VkDescriptorPool& GetDescriptorPool();
	VkInstance& GetInstance();
	
	GLFWwindow* GetWindow();

	VkFormat GetDepthFormat();
	VkImageView GetDepthImageView();

	std::vector<uint32_t>& GetQueueFamilyIndicesVector();
	
private:
	void createInstance();
	void initializeDebugMessenger();
	void createWindowSurface();
	void initializePhysicalDevice();
	void createLogicalDevice();
	
	void createCommandPools();

	void createFence();
	VkDescriptorSetLayout GetDescriptorSetLayout(const std::vector<VkDescriptorType>& descriptorTypes);
	VkRenderPass GetOrCreateRenderPass(
		const std::vector<VkFormat>& colorFormats,
		VkFormat depthFormat,
		icpRHIDepthAccess depthAccess);
	VkFramebuffer GetOrCreateFramebuffer(
		VkRenderPass renderPass,
		const std::vector<VkImageView>& attachments,
		uint32_t width,
		uint32_t height);
	void EndActiveRenderPass(VkCommandBuffer commandBuffer);
	void CleanUpRenderCaches();
	void CreateImGuiRenderPass();
	void CreateImGuiFramebuffers();
	void CleanUpImGuiFramebuffers();
	void CleanUpImGui();

	bool checkValidationLayerSupport();

	bool isDeviceSuitable(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkResult createDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT     debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

public:
	VkInstance m_instance{ VK_NULL_HANDLE };
	VkSurfaceKHR m_surface{ VK_NULL_HANDLE };

	VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
	QueueFamilyIndices m_queueIndices;

	std::vector<uint32_t> m_queueFamilyIndices;

	VkDevice m_device{ VK_NULL_HANDLE };
	VkQueue  m_graphicsQueue{ VK_NULL_HANDLE };
	VkQueue m_presentQueue{ VK_NULL_HANDLE };
	VkQueue m_transferQueue{ VK_NULL_HANDLE };
	VkQueue m_computeQueue{ VK_NULL_HANDLE };

	VkSwapchainKHR m_swapChain{ VK_NULL_HANDLE };
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkFormat m_swapChainImageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D m_swapChainExtent;

	VkCommandPool m_graphicsCommandPool{ VK_NULL_HANDLE };
	VkCommandPool m_transferCommandPool{ VK_NULL_HANDLE };
	VkCommandPool m_computeCommandPool{ VK_NULL_HANDLE };

	VkFormat m_depthFormat;
	VkImage m_depthImage;
	VmaAllocation m_depthBufferAllocation;

	VkImageView m_depthImageView;

	std::vector<VkFence> m_inFlightFences;
	std::vector<VkSemaphore> m_imageAvailableForRenderingSemaphores;
	std::vector<VkSemaphore> m_renderFinishedForPresentationSemaphores;

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorPool> m_frameDescriptorPools;

	VmaAllocator m_vmaAllocator{ VK_NULL_HANDLE };

private:
	GLFWwindow* m_window{ nullptr };
	uint32_t m_currentFrame{ 0 };
	uint32_t m_acquiredImageIndex{ 0 };
	bool m_hasAcquiredImage{ false };
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	std::vector<VkImageLayout> m_swapChainImageLayouts;
	std::unordered_map<std::string, VkRenderPass> m_renderPassCache;
	std::unordered_map<std::string, VkFramebuffer> m_framebufferCache;
	std::unordered_map<std::string, VkDescriptorSetLayout> m_descriptorSetLayoutCache;
	VkRenderPass m_activeRenderPass{ VK_NULL_HANDLE };
	VkFramebuffer m_activeFramebuffer{ VK_NULL_HANDLE };
	bool m_renderPassActive{ false };
	VkPipelineLayout m_activeGraphicsPipelineLayout{ VK_NULL_HANDLE };
	VkPipelineLayout m_activeComputePipelineLayout{ VK_NULL_HANDLE };
	VkRenderPass m_imguiRenderPass{ VK_NULL_HANDLE };
	std::vector<VkFramebuffer> m_imguiFramebuffers;
	bool m_imguiInitialized{ false };

	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
	bool m_enableValidationLayers = true;
	bool m_enableDebugUtilsLabel = true;

	// debug utilities label
	PFN_vkCmdBeginDebugUtilsLabelEXT m_vk_cmd_begin_debug_utils_label_ext = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT   m_vk_cmd_end_debug_utils_label_ext = nullptr;

	const std::vector<const char*> m_validationLayers{ "VK_LAYER_KHRONOS_validation" };
	std::vector<char const*> m_requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(__MACH__)
			"VK_KHR_portability_subset"
#endif
	};
};

INCEPTION_END_NAMESPACE
