#include "icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpConfigSystem::icpConfigSystem(const std::filesystem::path& configPath)
{
	m_configFilePath = configPath;
}

std::filesystem::path icpConfigSystem::getConfigFilePath()
{
	return m_configFilePath;
}

INCEPTION_END_NAMESPACE