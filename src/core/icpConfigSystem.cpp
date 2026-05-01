#include "icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpConfigSystem::icpConfigSystem(const std::filesystem::path& configPath)
{
	m_configFilePath = configPath;
#if defined(INCEPTION_RENDER_BACKEND_D3D12)
	m_shaderFolderPath = configPath.parent_path().parent_path() / "shaders" / "dxil";
#else
	m_shaderFolderPath = configPath.parent_path().parent_path() / "shaders" / "spv";
#endif
	m_imageResourcePath = configPath.parent_path().parent_path() / "resources\\textures";
	m_modelResourcePath = configPath.parent_path().parent_path() / "resources\\models";
}

std::filesystem::path icpConfigSystem::getConfigFilePath()
{
	return m_configFilePath;
}

INCEPTION_END_NAMESPACE
