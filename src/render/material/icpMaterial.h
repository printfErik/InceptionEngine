#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>


INCEPTION_BEGIN_NAMESPACE
class icpImageResource;

enum class eMaterialModel
{
	UNINITIALIZED = 0,
	LANBERMT,
	BLINNPHONG,
	PBR,
	MATERIAL_TYPE_COUNT
};


struct icpMaterialParameter
{
	
};

// pipeline: import img resource --> 

class icpMaterialTemplate
{
public:
	icpMaterialTemplate() = default;
	virtual ~icpMaterialTemplate() = default;

	virtual void createTextureImages() = 0;
	virtual void createTextureImageViews(size_t mipmaplevel) = 0;
	virtual void createTextureSampler() = 0;
private:
	eMaterialModel m_materialTemplateType = eMaterialModel::UNINITIALIZED;

};

class icpMaterialInstance : public icpMaterialTemplate
{
public:
	icpMaterialInstance() = default;
	virtual ~icpMaterialInstance() = default;

private:
	std::vector<icpMaterialTextureDescriptionInfo>
};

class icpOneTextureMaterial
{
public:
	icpOneTextureMaterial() = default;
	virtual ~icpOneTextureMaterial() = default;

	

	VkImage m_textureImage;
	VkDeviceMemory m_textureBufferMem;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	std::shared_ptr<icpImageResource> m_imgRes = nullptr;

	std::string m_imgId;
};

class icpBlinnPhongMaterial
{
public:
	icpBlinnPhongMaterial() = default;
	virtual ~icpBlinnPhongMaterial() = default;

	void createTextureImages();
	void createTextureImageViews(size_t mipmaplevel);
	void createTextureSampler();

	VkImage m_textureImage;
	VkDeviceMemory m_textureBufferMem;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	std::shared_ptr<icpImageResource> m_diffuseImgRes = nullptr;
	std::shared_ptr<icpImageResource> m_specularImgRes = nullptr;

	std::string m_diffuseMapTexResId;
	std::string m_specularMapTexResId;
};

INCEPTION_END_NAMESPACE