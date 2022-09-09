#pragma once
#include "../core/icpMacros.h"
#include "icpResourceBase.h"
#include <map>

INCEPTION_BEGIN_NAMESPACE

struct icpResourceContainer
{
	std::map<std::string, std::shared_ptr<icpResourceBase>> m_allResources;
};


class icpResourceSystem
{
public:

	icpResourceSystem();

	~icpResourceSystem();

	void loadImageResource(const std::filesystem::path& imgPath);
	void loadObjModelResource(const std::filesystem::path& objPath);

	icpResourceContainer m_resources;

};

INCEPTION_END_NAMESPACE