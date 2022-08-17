#pragma once

#include <iostream>
#include "icpMacros.h"
INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem;
class icpWindowSystem;
class InceptionEngine
{
public:

	InceptionEngine() = default;
	~InceptionEngine();

	bool initializeEngine();
	
	// game loop
	void startEngine();

	bool stopEngine();

private:
	std::shared_ptr<icpRenderSystem> m_renderSystem;
	std::shared_ptr<icpWindowSystem> m_windowSystem;
};

INCEPTION_END_NAMESPACE