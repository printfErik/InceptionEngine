#pragma once

#include "../core/icpMacros.h"
#include "icpSceneRenderer.h"

#include <array>
#include <memory>

INCEPTION_BEGIN_NAMESPACE

static constexpr uint32_t DEFERRED_CSM_CASCADE_COUNT = 4;
static constexpr uint32_t DEFERRED_CSM_RESOLUTION = 1024;

struct icpCSMCB
{
	glm::vec4 cascadeSplits = glm::vec4(0.f);
	glm::mat4 lightViewProj[DEFERRED_CSM_CASCADE_COUNT]{};
	glm::vec4 renderOptions = glm::vec4(1.f, 0.f, 0.f, 0.f);
};

class icpEditorUI;

class icpDeferredRenderer : public icpSceneRenderer
{
public:
	icpDeferredRenderer() = default;
	~icpDeferredRenderer() override;

	bool Initialize(std::shared_ptr<icpGPUDevice> rhi) override;
	void Cleanup() override;
	void Render() override;

private:
	void CreateSceneCB();
	void CreateCSMCB();
	void CreateCSMResources();
	void CreateRenderTargets();
	void CreateBindingSets();
	void CreatePipelines();
	void InitializeImGui();
	void ShutdownImGui();
	void UpdateSceneCB(uint32_t frameIndex);
	void UpdateCSMCB(uint32_t frameIndex);
	void UpdateMeshes(uint32_t frameIndex);
	void ShadowPass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex);
	void GBufferPass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex);
	uint64_t GTAOPass(uint32_t frameIndex);
	void CompositePass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex);
	void ForwardTranslucentPass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex);
	void DrawScene(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex);
	void DrawTranslucentScene(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex);
	void DrawShadowScene(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex, uint32_t cascadeIndex);
	void RenderImGui(std::shared_ptr<icpRHICommandList> commandList);

	std::shared_ptr<icpEditorUI> m_editorUI;
	std::shared_ptr<icpRHIPipeline> m_gbufferPipeline;
	std::shared_ptr<icpRHIPipeline> m_compositePipeline;
	std::shared_ptr<icpRHIPipeline> m_csmPipeline;
	std::shared_ptr<icpRHIPipeline> m_gtaoPipeline;
	std::shared_ptr<icpRHIPipeline> m_translucentPipeline;
	std::shared_ptr<icpRHITexture> m_gbufferA;
	std::shared_ptr<icpRHITexture> m_gbufferB;
	std::shared_ptr<icpRHITexture> m_gbufferC;
	std::shared_ptr<icpRHITexture> m_depth;
	std::shared_ptr<icpRHITexture> m_gtao;
	std::array<std::shared_ptr<icpRHITexture>, DEFERRED_CSM_CASCADE_COUNT> m_shadowMaps;
	std::shared_ptr<icpRHIBindingSet> m_compositeBindingSet;
	std::shared_ptr<icpRHIBindingSet> m_gtaoInputBindingSet;
	std::shared_ptr<icpRHIBindingSet> m_gtaoOutputBindingSet;
	std::vector<icpBufferRenderResource> m_csmUBOs;
	icpCSMCB m_csmData;
	perFrameCB m_frameCB;
	bool m_targetsValid = false;
	bool m_imguiInitialized = false;
};

INCEPTION_END_NAMESPACE
