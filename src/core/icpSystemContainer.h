#pragma once

#include "icpMacros.h"

INCEPTION_BEGIN_NAMESPACE


class icpConfigSystem;
class icpWindowSystem;
class icpRenderSystem;
class icpResourceSystem;
class icpCameraSystem;
class icpUiSystem;
class icpSceneSystem;
class icpLogSystem;

class icpSystemContainer 
{
public:

	void initializeAllSystems(const std::filesystem::path& _configFilePath);

	void shutdownAllSystems();

	std::shared_ptr<icpLogSystem> m_logSystem;
	std::shared_ptr<icpConfigSystem> m_configSystem;
	std::shared_ptr<icpWindowSystem> m_windowSystem;
	std::shared_ptr<icpCameraSystem> m_cameraSystem;
	std::shared_ptr<icpRenderSystem> m_renderSystem;
	std::shared_ptr<icpResourceSystem> m_resourceSystem;
	std::shared_ptr<icpUiSystem> m_uiSystem;
	std::shared_ptr<icpSceneSystem> m_sceneSystem;

};

extern icpSystemContainer g_system_container;


INCEPTION_END_NAMESPACE