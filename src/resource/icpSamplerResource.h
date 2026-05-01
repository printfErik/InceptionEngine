#pragma once
#include "../core/icpMacros.h"
#include "../resource/icpResourceBase.h"

INCEPTION_BEGIN_NAMESPACE

enum class icpSamplerFilter
{
	NEAREST = 0,
	LINEAR,
};

enum class icpSamplerAddressMode
{
	REPEAT = 0,
	CLAMP_TO_EDGE,
	MIRRORED_REPEAT,
};

class icpSamplerResource : public icpResourceBase
{
public:
	icpSamplerResource();
	~icpSamplerResource();

	icpSamplerFilter magFilter = icpSamplerFilter::NEAREST;
	icpSamplerFilter minFilter = icpSamplerFilter::NEAREST;
	icpSamplerAddressMode addressModeU = icpSamplerAddressMode::REPEAT;
	icpSamplerAddressMode addressModeV = icpSamplerAddressMode::REPEAT;
	icpSamplerAddressMode addressModeW = icpSamplerAddressMode::REPEAT;
};

INCEPTION_END_NAMESPACE
