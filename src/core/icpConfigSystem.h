#pragma once
#include "icpMacros.h"
#include <filesystem>

INCEPTION_BEGIN_NAMESPACE

class icpConfigSystem
{
public:
	icpConfigSystem(const std::filesystem::path& configPath);
	std::filesystem::path getConfigFilePath();

	std::filesystem::path m_configFilePath;
	std::filesystem::path m_shaderFolderPath;
	std::filesystem::path m_imageResourcePath;
	std::filesystem::path m_modelResourcePath;

	bool isDeferredRender = true;

	float NearPlane = 0.1f;
	float FarPlane = 100.f;
};


INCEPTION_END_NAMESPACE
