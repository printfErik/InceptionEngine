#pragma once

#include "icpSceneRenderer.h"
#include "material/icpTextureRenderResourceManager.h"
#include "RHI/D3D12/icpD3D12GPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

class icpD3D12DeferredRenderer : public icpSceneRenderer
{
public:
	bool Initialize(std::shared_ptr<icpGPUDevice> rhi) override;
	void Cleanup() override;
	void Render() override;

private:
	void CreateSceneCB();
	void CreateRenderTargets();
	void CreatePipelines();
	void CreateCompositeDescriptorTable();
	void UpdateSceneCB(uint32_t frameIndex);
	void UpdateMeshes(uint32_t frameIndex);
	void GBufferPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void CompositePass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);
	void DrawScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex);

	std::shared_ptr<icpD3D12GPUDevice> m_d3dDevice;
	std::shared_ptr<icpRHIPipeline> m_gbufferPipeline;
	std::shared_ptr<icpRHIPipeline> m_compositePipeline;
	std::shared_ptr<icpRHITexture> m_gbufferA;
	std::shared_ptr<icpRHITexture> m_gbufferB;
	std::shared_ptr<icpRHITexture> m_gbufferC;
	std::shared_ptr<icpRHITexture> m_depth;
	D3D12_GPU_DESCRIPTOR_HANDLE m_compositeSRVGpu{};
	bool m_targetsValid = false;
};

INCEPTION_END_NAMESPACE
