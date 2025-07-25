#pragma once
#include "../core/icpMacros.h"
#include "../mesh/icpMeshRendererComponent.h"

#include "RHI/icpGPUDevice.h"
#include "icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

class icpShadowManager;
class icpPrimitiveRendererComponent;

class icpRenderSystem
{
public:
	icpRenderSystem();
	virtual ~icpRenderSystem();

	bool initializeRenderSystem();
	void BuildRendererCompRenderResources();
	void drawFrame();
	void setFrameBufferResized(bool _isResized);

	void getAllStaticMeshRenderers();
	void drawCube();
	void createDirLight();

	std::shared_ptr<icpGPUDevice> GetGPUDevice();
	std::shared_ptr<icpSceneRenderer> GetSceneRenderer();
	std::shared_ptr<icpMaterialSubSystem> GetMaterialSubSystem();
	std::shared_ptr<icpTextureRenderResourceManager> GetTextureRenderResourceManager();

	std::shared_ptr<icpShadowManager> m_shadowManager = nullptr;
private:
	std::shared_ptr<icpGPUDevice> m_pGPUDevice = nullptr;
	std::shared_ptr<icpSceneRenderer> m_pSceneRenderer = nullptr;
	std::shared_ptr<icpTextureRenderResourceManager> m_textureRenderResourceManager = nullptr;
	std::shared_ptr<icpMaterialSubSystem> m_materialSystem = nullptr;
	

private:
};

INCEPTION_END_NAMESPACE