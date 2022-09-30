#pragma once
#include "../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

class icpSceneSystem
{
public:
	icpSceneSystem() = default;
	~icpSceneSystem() = default;

	void initializeScene(const std::filesystem::path& mapPath);

};


INCEPTION_END_NAMESPACE
