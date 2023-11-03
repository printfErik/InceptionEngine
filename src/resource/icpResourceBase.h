#pragma once

#include "../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

enum class icpResourceType
{
	UNKNOW = 0,
	MESH,
	TEXTURE,
	GLTF,
	MATERIL
};

class icpResourceBase
{
public:
	icpResourceBase();
	virtual ~icpResourceBase();

	icpResourceType m_resType = icpResourceType::UNKNOW;
	std::string m_id; // todo temp

};

INCEPTION_END_NAMESPACE