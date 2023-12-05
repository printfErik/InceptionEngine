#include "icpDeferredCompositePass.h"

INCEPTION_BEGIN_NAMESPACE

void icpDeferredCompositePass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	CreateDescriptorSetLayouts();
	SetupPipeline();
}

void icpDeferredCompositePass::SetupPipeline()
{

}

INCEPTION_END_NAMESPACE