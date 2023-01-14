#pragma once
#include "../core/icpMacros.h"
#include "../mesh/icpMeshRendererComponent.h"

#include "icpRHI.h"
#include "icpRenderPassManager.h"

INCEPTION_BEGIN_NAMESPACE
class icpPrimitiveRendererComponment;

class icpRenderSystem
{
public:
	icpRenderSystem();
	~icpRenderSystem();

	bool initializeRenderSystem();
	void drawFrame();
	void setFrameBufferResized(bool _isResized);

	void getAllStaticMeshRenderers();

	std::shared_ptr<icpRHIBase> m_rhi;
	std::shared_ptr<icpRenderPassManager> m_renderPassManager;

	void drawCube();
	void createDirLight();

private:
};

INCEPTION_END_NAMESPACE