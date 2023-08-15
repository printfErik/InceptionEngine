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

struct icpScalaMaterialParameterInfo
{
	std::string m_strScalaName;
	float m_fValue = 0.f;
};

// todo: replace string with real guid
typedef std::string TextureID;

struct icpTextureMaterialParameterInfo
{
	std::string m_strTextureName;
	TextureID m_textureID;
};

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
	//virtual void addDiffuseTexture(const std::string& texID) = 0;
	//virtual void addSpecularTexture(const std::string& texID) = 0;
	virtual void AddTexture(const std::string& texID) = 0;
	virtual void AddScalaValue(const icpScalaMaterialParameterInfo& value) = 0;
	virtual void SetupMaterialRenderResources() = 0;

	virtual uint32_t GetSRVNumber() const = 0;


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
	//void addDiffuseTexture(const std::string& texID) override;
	//void addSpecularTexture(const std::string& texID) override;
	void AddTexture(const std::string& texID) override;
	void AddScalaValue(const icpScalaMaterialParameterInfo& value) override;
	void SetupMaterialRenderResources() override;
	uint64_t ComputeUBOSize();

	uint32_t GetSRVNumber() const override;
private:
	
	std::vector<icpScalaMaterialParameterInfo> m_vScalarParameterValues;
	std::vector<bool> m_vBoolParameterValues;
	std::vector<glm::vec4> m_vVectorParameterValues;
	std::vector<icpTextureMaterialParameterInfo> m_vTextureParameterValues;

	uint32_t m_nSRVs = 0;
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

	std::vector<std::shared_ptr<icpMaterialTemplate>> m_vMaterialContainer;

};

INCEPTION_END_NAMESPACE