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

	icpResourceContainer m_resources;

};

INCEPTION_END_NAMESPACE