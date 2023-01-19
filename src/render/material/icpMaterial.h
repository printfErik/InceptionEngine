#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>


INCEPTION_BEGIN_NAMESPACE
class icpTextureRenderResourceManager;
class icpImageResource;

enum class eMaterialModel
{
	UNINITIALIZED = 0,
	LAMBERT,
	BLINNPHONG,
	PBR,
	MATERIAL_TYPE_COUNT
};

// pipeline: import img resource --> 

class icpMaterialTemplate
{
public:
	icpMaterialTemplate() = default;
	virtual ~icpMaterialTemplate() = default;
	virtual void allocateDescriptorSets() = 0;
	virtual void createUniformBuffers() = 0;
protected:
	eMaterialModel m_materialTemplateType = eMaterialModel::UNINITIALIZED;

	VkDescriptorSet m_perMaterialDS{VK_NULL_HANDLE};

	VkBuffer m_perMaterialUniformBuffers{ VK_NULL_HANDLE };
	VkDeviceMemory m_perMaterialUniformBufferMem{ VK_NULL_HANDLE };

};

class icpLambertMaterialInstance : public icpMaterialTemplate
{
public:

	struct UBOPerMaterial
	{
		float shininess;
	};

	icpLambertMaterialInstance();
	virtual ~icpLambertMaterialInstance() = default;
	void allocateDescriptorSets() override;
	void createUniformBuffers() override;

private:
	std::vector<std::string> m_texRenderResourceIDs;
};

class icpBlinnPhongMaterialInstance
{
	
};

class icpPBRMaterialInstance
{
	
};

INCEPTION_END_NAMESPACE