#pragma once

#include <iostream>
#include "icpMacros.h"
#include <filesystem>
INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem;
class icpWindowSystem;
class icpConfigSystem;

class InceptionEngine
{
public:

	InceptionEngine() = default;
	~InceptionEngine();

	bool initializeEngine(const std::filesystem::path& _configFilePath);
	
	// game loop
	void startEngine();

	bool stopEngine();

private:
	std::shared_ptr<icpRenderSystem> m_renderSystem;
	std::shared_ptr<icpWindowSystem> m_windowSystem;
	std::shared_ptr<icpConfigSystem> m_configSystem;
};

INCEPTION_END_NAMESPACE