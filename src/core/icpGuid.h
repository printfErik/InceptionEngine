#pragma once
#include "icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

class icpGuid
{
public:
	icpGuid();
	icpGuid(uint64_t guid);

	icpGuid(const icpGuid&) = default;

	operator uint64_t() const { return m_guid; }

private:

	uint64_t m_guid;
};


INCEPTION_END_NAMESPACE

namespace std
{
	template<>
	struct hash<Inception::icpGuid>
	{
		std::size_t operator()(const Inception::icpGuid& uuid) const
		{
			return (uint64_t)uuid;
		}
	};
}