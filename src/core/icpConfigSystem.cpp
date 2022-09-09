#include "icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpConfigSystem::icpConfigSystem(const std::filesystem::path& configPath)
{
	m_configFilePath = configPath;
	m_shaderFolderPath = configPath.parent_path().parent_path() / "shaders";
	m_imageResourcePath = configPath.parent_path().parent_path() / "resources\\textures";
	m_modelResourcePath = configPath.parent_path().parent_path() / "resources\\models";
}

std::filesystem::path icpConfigSystem::getConfigFilePath()
{
	return m_configFilePath;
}

INCEPTION_END_NAMESPACE