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
	std::filesystem::path m_shaderPath;
	std::filesystem::path m_texturePath;
};


INCEPTION_END_NAMESPACE
