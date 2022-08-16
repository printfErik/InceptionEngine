#pragma once

#include <iostream>
#include "icpMacros.h"
INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem;

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
};

INCEPTION_END_NAMESPACE