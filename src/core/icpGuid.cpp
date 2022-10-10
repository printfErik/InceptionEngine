#include "icpGuid.h"

#include <random>
#include <unordered_map>

INCEPTION_BEGIN_NAMESPACE

static std::random_device s_randomDevice;
static std::mt19937_64 s_randomEngine(s_randomDevice());
static std::uniform_int_distribution<uint64_t> s_uniformDis;

icpGuid::icpGuid()
	: m_guid(s_uniformDis(s_randomDevice))
{

}

icpGuid::icpGuid(uint64_t guid)
	: m_guid(guid)
{

}


INCEPTION_END_NAMESPACE