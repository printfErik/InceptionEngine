#pragma once
#include "icpMacros.h"

#include <spdlog/spdlog.h>

#include <spdlog/async_logger.h>

INCEPTION_BEGIN_NAMESPACE

class icpLogSystem
{
public:
	icpLogSystem();
	virtual ~icpLogSystem();

	std::shared_ptr<spdlog::logger> getLogger();
private:

	std::shared_ptr<spdlog::logger> m_logger;
};



INCEPTION_END_NAMESPACE