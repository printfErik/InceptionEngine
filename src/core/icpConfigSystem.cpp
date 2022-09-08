#include "icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpConfigSystem::icpConfigSystem(const std::filesystem::path& configPath)
{
	m_configFilePath = configPath;
	m_shaderPath = configPath.parent_path().parent_path() / "shaders";
	m_texturePath = configPath.parent_path().parent_path() / "resources\\textures";
}

std::filesystem::path icpConfigSystem::getConfigFilePath()
{
	return m_configFilePath;
}

INCEPTION_END_NAMESPACE