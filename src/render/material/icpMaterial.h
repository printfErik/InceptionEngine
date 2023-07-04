#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

#include "../RHI/Vulkan/vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE
class icpTextureRenderResourceManager;
class icpImageResource;

enum class eMaterialModel
{
	NULL_MATERIAL = 0,
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
	virtual void addDiffuseTexture(const std::string& texID) = 0;
	virtual void addSpecularTexture(const std::string& texID) = 0;
	virtual void addShininess(float shininess) = 0;
	virtual void setupMaterialRenderResources() = 0;

protected:
	eMaterialModel m_materialTemplateType = eMaterialModel::NULL_MATERIAL;

	std::vector<VkDescriptorSet> m_perMaterialDSs;

	std::vector<VkBuffer> m_perMaterialUniformBuffers;
	std::vector<VmaAllocation> m_perMaterialUniformBufferAllocations;

};

class icpLambertMaterialInstance : public icpMaterialTemplate
{
public:
	icpLambertMaterialInstance();
	virtual ~icpLambertMaterialInstance() = default;

	void allocateDescriptorSets() override {}
	void createUniformBuffers() override {}
	void addDiffuseTexture(const std::string& texID) override {}
	void addSpecularTexture(const std::string& texID) override {}
	void addShininess(float shininess) override {}
	void setupMaterialRenderResources() override {}

private:
	std::vector<std::string> m_texRenderResourceIDs;
};

class icpBlinnPhongMaterialInstance : public icpMaterialTemplate
{
public:
	struct UBOPerMaterial
	{
		float shininess;
	};

	icpBlinnPhongMaterialInstance();
	virtual ~icpBlinnPhongMaterialInstance() = default;

	void allocateDescriptorSets() override;
	void createUniformBuffers() override;
	void addDiffuseTexture(const std::string& texID) override;
	void addSpecularTexture(const std::string& texID) override;
	void addShininess(float shininess) override;
	void setupMaterialRenderResources() override;
private:
	std::vector<std::string> m_texRenderResourceIDs;

	UBOPerMaterial m_ubo{};

};

class icpNullMaterialInstance : public icpMaterialTemplate
{
public:
	icpNullMaterialInstance();
	virtual ~icpNullMaterialInstance() = default;

	void allocateDescriptorSets() override {}
	void createUniformBuffers() override {}
	void addDiffuseTexture(const std::string& texID) override {}
	void addSpecularTexture(const std::string& texID) override {}
	void addShininess(float shininess) override {}
	void setupMaterialRenderResources() override {}
};

class icpPBRMaterialInstance
{
	
};

class icpMaterialSubSystem
{
public:
	icpMaterialSubSystem() = default;
	virtual ~icpMaterialSubSystem() = default;

	void initializeMaterialSubSystem();
	std::shared_ptr<icpMaterialTemplate> createMaterialInstance(eMaterialModel materialType);

	std::vector<std::shared_ptr<icpMaterialTemplate>> m_materials;

};

INCEPTION_END_NAMESPACE