#pragma once

#include <memory>
#include <vector>
#include <filesystem>
#include <string>
#include <array>

#define INCEPTION_BEGIN_NAMESPACE namespace Inception {

#define INCEPTION_END_NAMESPACE }

#define ICP_LOG_INFO(...) g_system_container.m_logSystem->getLogger()->info(__VA_ARGS__)
#define ICP_LOG_DEBUG(...) g_system_container.m_logSystem->getLogger()->debug(__VA_ARGS__)
#define ICP_LOG_WARING(...) g_system_container.m_logSystem->getLogger()->warn(__VA_ARGS__)
#define ICP_LOG_ERROR(...) g_system_container.m_logSystem->getLogger()->error(__VA_ARGS__)
#define ICP_LOG_FATAL(...) g_system_container.m_logSystem->getLogger()->critical(__VA_ARGS__)
