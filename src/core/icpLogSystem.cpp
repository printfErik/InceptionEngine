#include "icpLogSystem.h"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

INCEPTION_BEGIN_NAMESPACE

icpLogSystem::icpLogSystem()
{
	auto consoleColorSink = std::make_shared<::spdlog::sinks::stdout_color_sink_mt>();
	consoleColorSink->set_level(spdlog::level::trace);
	consoleColorSink->set_pattern("[%^%l%$] %v");

	const spdlog::sinks_init_list sinkList = { consoleColorSink };

	m_logger = std::make_shared<spdlog::async_logger>("Inception Logger",
		sinkList.begin(),
		sinkList.end(),
		spdlog::thread_pool(),
		spdlog::async_overflow_policy::block);

	m_logger->set_level(spdlog::level::trace);

	spdlog::register_logger(m_logger);
}

icpLogSystem::~icpLogSystem()
{
	spdlog::drop_all();
}

std::shared_ptr<spdlog::logger> icpLogSystem::getLogger()
{
	return m_logger;
}


INCEPTION_END_NAMESPACE