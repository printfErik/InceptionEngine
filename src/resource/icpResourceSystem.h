#pragma once
#include "../core/icpMacros.h"
#include "icpResourceBase.h"
#include <map>

INCEPTION_BEGIN_NAMESPACE

struct icpResourceContainer
{
	std::map<icpResourceType, std::map<std::string, std::shared_ptr<icpResourceBase>>> m_allResources;
};


class icpResourceSystem
{
public:

	icpResourceSystem();

	~icpResourceSystem();

	std::shared_ptr<icpResourceBase> loadImageResource(const std::filesystem::path& imgPath);
	std::shared_ptr<icpResourceBase> loadObjModelResource(const std::filesystem::path& objPath, bool ifLoadRelatedImgRes = false);

	icpResourceContainer m_resources;

};

INCEPTION_END_NAMESPACE