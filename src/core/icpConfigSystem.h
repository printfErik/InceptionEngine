#pragma once
#include "icpMacros.h"
#include <filesystem>

INCEPTION_BEGIN_NAMESPACE

class icpConfigSystem
{
public:
	icpConfigSystem(const std::filesystem::path& configPath);

private:

	std::filesystem::path m_configFilePath;
};


INCEPTION_END_NAMESPACE