#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

#include "vec4.hpp"
#include "../RHI/Vulkan/vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE
class icpTextureRenderResourceManager;
class icpImageResource;

/*
enum class eMaterialModel
{
	NULL_MATERIAL = 0,
	LAMBERT,
	BLINNPHONG,
	PBR,
	MATERIAL_TYPE_COUNT
};
*/
// pipeline: import img resource -->

// todo: replace string with real guid
typedef std::string TextureID;

// how to interpret values container
enum class eMaterialShadingModel
{
	UNLIT = 0,
	DEFAULT_LIT,
	SHADING_MODEL_COUNT
};


class icpMaterialTemplate
{
public:
	icpMaterialTemplate() = default;
	icpMaterialTemplate(eMaterialShadingModel shading_model){}
	virtual ~icpMaterialTemplate() = default;
	virtual void AllocateDescriptorSets() = 0;
	virtual void CreateUniformBuffers() = 0;
	virtual void addDiffuseTexture(const std::string& texID) = 0;
	virtual void addSpecularTexture(const std::string& texID) = 0;
	virtual void addShininess(float shininess) = 0;
	virtual void setupMaterialRenderResources() = 0;

	// return a mapped CPU address
	virtual void* MapUniformBuffer(int index) = 0;


	eMaterialShadingModel m_shadingModel = eMaterialShadingModel::SHADING_MODEL_COUNT;

	std::vector<VkDescriptorSet> m_perMaterialDSs;

	std::vector<VkBuffer> m_perMaterialUniformBuffers;
	std::vector<VmaAllocation> m_perMaterialUniformBufferAllocations;
};

class icpMaterialInstance : public icpMaterialTemplate
{
public:
	icpMaterialInstance() = default;
	icpMaterialInstance(eMaterialShadingModel shading_model);
	virtual ~icpMaterialInstance() = default;

	void AllocateDescriptorSets() override;
	void CreateUniformBuffers() override;
	void addDiffuseTexture(const std::string& texID) override;
	void addSpecularTexture(const std::string& texID) override;
	void addShininess(float shininess) override;
	void setupMaterialRenderResources() override;
	void* MapUniformBuffer(int index) override;
	uint64_t ComputeUBOSize();
private:
	
	std::vector<float> m_scalarParameterValues;
	std::vector<bool> m_boolParameterValues;
	std::vector<glm::vec4> m_vectorParameterValues;
	std::vector<TextureID> m_textureParameterValues;
};

/*
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
		float shininess = 1.f;
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
*/
class icpMaterialSubSystem
{
public:
	icpMaterialSubSystem() = default;
	virtual ~icpMaterialSubSystem() = default;

	void initializeMaterialSubSystem();
	std::shared_ptr<icpMaterialTemplate> createMaterialInstance(eMaterialShadingModel shadingModel);

	std::vector<std::shared_ptr<icpMaterialTemplate>> m_materials;

};

INCEPTION_END_NAMESPACE