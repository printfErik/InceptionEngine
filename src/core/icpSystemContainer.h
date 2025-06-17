#pragma once

#include "icpMacros.h"


namespace Inception
{
	class icpShadowManager;
}

INCEPTION_BEGIN_NAMESPACE
	class icpConfigSystem;
class icpWindowSystem;
class icpRenderSystem;
class icpResourceSystem;
class icpCameraSystem;
class icpUiSystem;
class icpSceneSystem;
class icpLogSystem;
class icpLightSystem;

class icpSystemContainer 
{
public:

	void initializeAllSystems(const std::filesystem::path& _configFilePath);

	void shutdownAllSystems();

	std::shared_ptr<icpLogSystem> m_logSystem = nullptr;
	std::shared_ptr<icpConfigSystem> m_configSystem = nullptr;
	std::shared_ptr<icpWindowSystem> m_windowSystem = nullptr;
	std::shared_ptr<icpCameraSystem> m_cameraSystem = nullptr;
	std::shared_ptr<icpLightSystem> m_lightSystem = nullptr;
	std::shared_ptr<icpShadowManager> m_shadowSystem = nullptr;
	std::shared_ptr<icpRenderSystem> m_renderSystem = nullptr;
	std::shared_ptr<icpResourceSystem> m_resourceSystem = nullptr;
	std::shared_ptr<icpUiSystem> m_uiSystem = nullptr;
	std::shared_ptr<icpSceneSystem> m_sceneSystem = nullptr;

};

extern icpSystemContainer g_system_container;


INCEPTION_END_NAMESPACE