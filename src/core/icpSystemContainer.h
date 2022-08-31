#pragma once

#include "icpMacros.h"

INCEPTION_BEGIN_NAMESPACE


class icpConfigSystem;
class icpWindowSystem;
class icpRenderSystem;

class icpSystemContainer 
{
public:

	void initializeAllSystems(const std::filesystem::path& _configFilePath);

	void shutdownAllSystems();

	std::shared_ptr<icpConfigSystem> m_configSystem;
	std::shared_ptr<icpWindowSystem> m_windowSystem;
	std::shared_ptr<icpRenderSystem> m_renderSystem;

};

extern icpSystemContainer g_system_container;


INCEPTION_END_NAMESPACE