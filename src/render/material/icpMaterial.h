#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>


INCEPTION_BEGIN_NAMESPACE
class icpImageResource;

enum class eMaterialModel
{
	BlinnPhong = 0,
	ONE_TEXTURE_ONLY,
	PBR,
	MATERIAL_TYPE_COUNT
};

class icpMaterialBase
{
public:
	icpMaterialBase();
	virtual ~icpMaterialBase() = default;

	virtual void createTextureImages() = 0;
	virtual void createTextureImageViews(size_t mipmaplevel) = 0;
	virtual void createTextureSampler();
};

class icpOneTextureMaterial : public icpMaterialBase
{
public:
	icpOneTextureMaterial() = default;
	virtual ~icpOneTextureMaterial() = default;

	void createTextureImages() override;
	void createTextureImageViews(size_t mipmaplevel) override;
	void createTextureSampler() override;

	VkImage m_textureImage;
	VkDeviceMemory m_textureBufferMem;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	std::shared_ptr<icpImageResource> m_imgRes = nullptr;

	std::string m_imgId;
};

class icpBlinnPhongMaterial : public icpMaterialBase
{
public:
	icpBlinnPhongMaterial() = default;
	virtual ~icpBlinnPhongMaterial() = default;

	void createTextureImages() override;
	void createTextureImageViews(size_t mipmaplevel) override;
	void createTextureSampler() override;

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