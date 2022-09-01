#pragma once

#include <iostream>
#include "core/icpMacros.h"
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
};

INCEPTION_END_NAMESPACE