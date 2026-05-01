#pragma once

#include "icpSceneRenderer.h"
#include "material/icpTextureRenderResourceManager.h"
#include "RHI/D3D12/icpD3D12GPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

static constexpr uint32_t D3D12_CSM_CASCADE_COUNT = 4;
static constexpr uint32_t D3D12_CSM_RESOLUTION = 1024;

struct icpD3D12CSMCB
{
	glm::vec4 cascadeSplits = glm::vec4(0.f);
	glm::mat4 lightViewProj[D3D12_CSM_CASCADE_COUNT]{};
	glm::vec4 renderOptions = glm::vec4(1.f, 0.f, 0.f, 0.f);
};

class icpEditorUI;

class icpD3D12DeferredRenderer : public icpSceneRenderer
{
public:
	~icpD3D12DeferredRenderer() override;
	bool Initialize(std::shared_ptr<icpGPUDevice> rhi) override;
	void Cleanup() override;
	void Render() override;

private:
	void CreateSceneCB();
	void CreateCSMCB();
	void CreateCSMResources();
	void CreateRenderTargets();
	void CreatePipelines();
	void CreateCompositeDescriptorTable();
	void CreateGTAODescriptorTable();
	void InitializeImGui();
	void ShutdownImGui();
	void UpdateSceneCB(uint32_t frameIndex);
	void UpdateCSMCB(uint32_t frameIndex);
	void UpdateMeshes(uint32_t frameIndex);
	void ShadowPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void GBufferPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	uint64_t GTAOPass(uint32_t frameIndex);
	void CompositePass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void ForwardTranslucentPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void DrawScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void DrawTranslucentScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void DrawShadowScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex, uint32_t cascadeIndex);
	void RenderImGui(ID3D12GraphicsCommandList* cmd);

	std::shared_ptr<icpD3D12GPUDevice> m_d3dDevice;
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
	std::shared_ptr<icpRHITexture> m_shadowMaps[D3D12_CSM_CASCADE_COUNT];
	std::vector<icpBufferRenderResource> m_csmUBOs;
	icpD3D12CSMCB m_csmData;
	perFrameCB m_frameCB;
	D3D12_CPU_DESCRIPTOR_HANDLE m_imguiFontSRVCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_imguiFontSRVGpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_compositeSRVGpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gtaoSRVGpu{};
	bool m_targetsValid = false;
	bool m_imguiInitialized = false;
};

INCEPTION_END_NAMESPACE
