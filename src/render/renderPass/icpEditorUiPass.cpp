#include "icpEditorUiPass.h"
#include "../../render/icpVulkanUtility.h"

INCEPTION_BEGIN_NAMESPACE

void icpEditorUiPass::initializeRenderPass(RendePassInitInfo initInfo)
{
	m_rhi = initInfo.rhi;

	m_editorUI = initInfo.editorUi;

	createViewPortImage();

	createRenderPass();
	setupPipeline();
	createFrameBuffers();
}


void icpEditorUiPass::createViewPortImage()
{
	m_viewPortImages.resize(m_rhi->m_swapChainImages.size());
	m_viewPortImageDeviceMemory.resize(m_rhi->m_swapChainImages.size());

	for (uint32_t i = 0; i < m_rhi->m_swapChainImages.size(); i++)
	{
		// Create the linear tiled destination image to copy to and to read the memory from
		VkImageCreateInfo imageCreateCI{};
		imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
		// Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
		imageCreateCI.format = VK_FORMAT_B8G8R8A8_SRGB;
		imageCreateCI.extent.width = m_rhi->m_swapChainExtent.width;
		imageCreateCI.extent.height = m_rhi->m_swapChainExtent.height;
		imageCreateCI.extent.depth = 1;
		imageCreateCI.arrayLayers = 1;
		imageCreateCI.mipLevels = 1;
		imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		// Create the image
		// VkImage dstImage;
		vkCreateImage(m_rhi->m_device, &imageCreateCI, nullptr, &m_viewPortImages[i]);
		// Create memory to back up the image
		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		// VkDeviceMemory dstImageMemory;
		vkGetImageMemoryRequirements(m_rhi->m_device, m_viewPortImages[i], &memRequirements);
		memAllocInfo.allocationSize = memRequirements.size;
		// Memory must be host visible to copy from
		memAllocInfo.memoryTypeIndex = icpVulkanUtility::findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_rhi->m_physicalDevice);
		vkAllocateMemory(m_rhi->m_device, &memAllocInfo, nullptr, &m_viewPortImageDeviceMemory[i]);
		vkBindImageMemory(m_rhi->m_device, m_viewPortImages[i], m_viewPortImageDeviceMemory[i], 0);

		VkCommandBuffer copyCmd = icpVulkanUtility::beginSingleTimeCommands(m_rhi->m_uiCommandPool, m_rhi->m_device);

		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.image = m_viewPortImages[i];
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		icpVulkanUtility::endSingleTimeCommandsAndSubmit(copyCmd, m_rhi->m_transferQueue, m_rhi->m_uiCommandPool, m_rhi->m_device);
	}

	m_viewPortImageViews.resize(m_viewPortImages.size());

	for (uint32_t i = 0; i < m_viewPortImages.size(); i++)
	{
		m_viewPortImageViews[i] = icpVulkanUtility::createImageView(m_viewPortImages[i], VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_rhi->m_device);
	}
}

void icpEditorUiPass::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 1> attachments{ colorAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_rhi->m_device, &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void icpEditorUiPass::setupPipeline()
{
	
}

void icpEditorUiPass::cleanup()
{
	
}

void icpEditorUiPass::cleanupSwapChain()
{
	
}

icpEditorUiPass::~icpEditorUiPass()
{
	
}

void icpEditorUiPass::createFrameBuffers()
{
	
}

void icpEditorUiPass::recreateSwapChain()
{
	
}


void icpEditorUiPass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	
}

void icpEditorUiPass::render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info)
{
	
}


INCEPTION_END_NAMESPACE