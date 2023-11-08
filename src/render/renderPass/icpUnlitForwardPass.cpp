#include "icpUnlitForwardPass.h"

INCEPTION_BEGIN_NAMESPACE

icpUnlitForwardPass::~icpUnlitForwardPass()
{
	
}

void icpUnlitForwardPass::initializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;

	CreateDescriptorSetLayouts();
	//CreateSceneCB();
	AllocateDescriptorSets();
	AllocateCommandBuffers();
	//createRenderPass();
	setupPipeline();
	//createFrameBuffers();
}

void icpUnlitForwardPass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();
	// per mesh
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding perObjectSSBOBinding{};
		perObjectSSBOBinding.binding = 0;
		perObjectSSBOBinding.descriptorCount = 1;
		perObjectSSBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectSSBOBinding.pImmutableSamplers = nullptr;
		perObjectSSBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_MESH].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectSSBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eUnlitForwardPassDSType::PER_MESH].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// per Unlit Material
	{
		// set 1, binding 0 
		VkDescriptorSetLayoutBinding perMaterialUBOBinding{};
		perMaterialUBOBinding.binding = 0;
		perMaterialUBOBinding.descriptorCount = 1;
		perMaterialUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perMaterialUBOBinding.pImmutableSamplers = nullptr;
		perMaterialUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::vector<VkDescriptorSetLayoutBinding> bindings {perMaterialUBOBinding};

		// set 1, binding 1
		VkDescriptorSetLayoutBinding baseColorTextureSamplerLayoutBinding{};
		baseColorTextureSamplerLayoutBinding.binding = 1;
		baseColorTextureSamplerLayoutBinding.descriptorCount = 1;
		baseColorTextureSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		baseColorTextureSamplerLayoutBinding.pImmutableSamplers = nullptr;
		baseColorTextureSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		bindings.push_back(baseColorTextureSamplerLayoutBinding);

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eUnlitForwardPassDSType::PER_MATERIAL].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// perFrame
	{
		// set 2, binding 0 
		VkDescriptorSetLayoutBinding perFrameUBOBinding{};
		perFrameUBOBinding.binding = 0;
		perFrameUBOBinding.descriptorCount = 1;
		perFrameUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perFrameUBOBinding.pImmutableSamplers = nullptr;
		perFrameUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_FRAME].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perFrameUBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eUnlitForwardPassDSType::PER_FRAME].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

}



INCEPTION_END_NAMESPACE