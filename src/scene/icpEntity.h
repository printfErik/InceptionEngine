#pragma once

#include "../core/icpMacros.h"
#include "icpComponent.h"

INCEPTION_BEGIN_NAMESPACE

class icpGameEntity
{
public:
	icpGameEntity();
	virtual ~icpGameEntity() = 0;

	void registerComponent(std::weak_ptr<icpComponentBase> comp);
	void uninstallComponent(std::weak_ptr<icpComponentBase> comp);


private:


	std::vector<std::weak_ptr<icpComponentBase>> m_components;

};


INCEPTION_END_NAMESPACE