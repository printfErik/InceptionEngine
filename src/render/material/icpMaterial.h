#pragma once

#include <map>

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

#include <glm/vec4.hpp>
#include <vk_mem_alloc.h>

#include "icpTextureRenderResourceManager.h"
#include "glm/vec3.hpp"

INCEPTION_BEGIN_NAMESPACE
	class icpTextureRenderResourceManager;
class icpImageResource;


struct icpScalaMaterialParameterInfo
{
	float m_fValue = 0.f;
};

struct icpVector4MaterialParameterInfo
{
	glm::vec4 m_vValue{};
};

// todo: replace string with real guid
typedef std::string TextureID;

struct icpTextureMaterialParameterInfo
{
	TextureID m_textureID;
};

// how to interpret values container
enum class eMaterialShadingModel
{
	UNLIT = 0,
	PBR_LIT,
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
	virtual void AddTexture(const std::string& key, const icpTextureMaterialParameterInfo& textureInfo) = 0;
	virtual void AddScalaValue(const std::string& key, const icpScalaMaterialParameterInfo& value) = 0;
	virtual void AddVector4Value(const std::string& key, const icpVector4MaterialParameterInfo& value) = 0;
	virtual void SetupMaterialRenderResources() = 0;
	virtual void MemCopyToBuffer(void* dst, void* src, size_t size) = 0;
	virtual uint32_t GetSRVNumber() const = 0;
	virtual void* CheckMaterialDataCache() = 0;

	eMaterialShadingModel m_shadingModel = eMaterialShadingModel::SHADING_MODEL_COUNT;

	std::vector<VkDescriptorSet> m_perMaterialDSs;

	std::vector<VkBuffer> m_perMaterialUniformBuffers;
	std::vector<VmaAllocation> m_perMaterialUniformBufferAllocations;

	bool m_bRenderResourcesReady = false;
};

struct alignas(16) PBRShaderMaterial
{
	glm::vec4 baseColorFactor = glm::vec4(1.f);
	glm::vec4 emissiveFactor = glm::vec4(0.f);
	//glm::vec4 diffuseFactor = glm::vec4(1.f);
	//glm::vec4 specularFactor = glm::vec4(1.f);
	//float workflow = 0.f;
	float colorTextureSet = -1;
	float PhysicalDescriptorTextureSet = -1;
	float metallicTextureSet = -1;
	float roughnessTextureSet = -1;
	float normalTextureSet = -1;
	float occlusionTextureSet = -1;
	float emissiveTextureSet = -1;
	float metallicFactor = 1.f;
	float roughnessFactor = 1.f;
	float alphaMask = 0.f;
	float alphaMaskCutoff = 0.f;
	//float emissiveStrength = 1.f;
};

struct alignas(16) UnlitShaderMaterial
{
	glm::vec4 baseColorFactor = glm::vec4(1.f);
	float colorTextureSet = -1;
};

class icpMaterialInstance : public icpMaterialTemplate
{
public:
	icpMaterialInstance() = default;
	icpMaterialInstance(eMaterialShadingModel shading_model);
	~icpMaterialInstance() override = default ;

	void AllocateDescriptorSets() override;
	void CreateUniformBuffers() override;
	void AddTexture(const std::string& key, const icpTextureMaterialParameterInfo& textureInfo) override;
	void AddScalaValue(const std::string& key, const icpScalaMaterialParameterInfo& value) override;
	void AddVector4Value(const std::string& key, const icpVector4MaterialParameterInfo& value) override;
	void SetupMaterialRenderResources() override;
	uint64_t ComputeUBOSize();
	void MemCopyToBuffer(void* dst, void* src, size_t size) override;
	uint32_t GetSRVNumber() const override;
	void* CheckMaterialDataCache() override;

private:

	void FillPBRDataCache();
	void AddedTextureDescriptor(const std::string& textureType, std::vector<std::vector<icpTextureRenderResourceInfo>>& imgInfosAllFrames);
	std::map<std::string, icpScalaMaterialParameterInfo> m_vScalarParameterValues;
	std::map<std::string, bool> m_vBoolParameterValues;
	std::map<std::string, icpVector4MaterialParameterInfo> m_vVectorParameterValues;
	std::map<std::string, icpTextureMaterialParameterInfo> m_vTextureParameterValues;

	uint32_t m_nSRVs = 0;

	PBRShaderMaterial m_pbrDataCache;
};

class icpMaterialSubSystem
{
public:
	icpMaterialSubSystem() = default;
	virtual ~icpMaterialSubSystem() = default;

	void initializeMaterialSubSystem();
	std::shared_ptr<icpMaterialTemplate> createMaterialInstance(eMaterialShadingModel shadingModel);

	void PrepareMaterialRenderResources();

	std::vector<std::shared_ptr<icpMaterialTemplate>> m_vMaterialContainer;

};

INCEPTION_END_NAMESPACE