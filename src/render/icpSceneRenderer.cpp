#include "icpSceneRenderer.h"
#include "renderPass/icpRenderPassBase.h"


INCEPTION_BEGIN_NAMESPACE

std::shared_ptr<icpRenderPassBase> icpSceneRenderer::AccessRenderPass(eRenderPass passType)
{
	return m_renderPasses[static_cast<int>(passType)];
}

void icpSceneRenderer::Cleanup()
{
	for (const auto renderPass : m_renderPasses)
	{
		renderPass->Cleanup();
	}
}



INCEPTION_END_NAMESPACE