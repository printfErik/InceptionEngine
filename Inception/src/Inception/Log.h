#pragma once
#include <memory>
#include "Core.h"
#include "spdlog/spdlog.h"
 


namespace Inception {
	class INCEPTION_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define ICP_CORE_ERROR(...) ::Inception::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ICP_CORE_INFO(...) ::Inception::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ICP_CORE_TRACE(...) ::Inception::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ICP_CORE_WARN(...) ::Inception::Log::GetCoreLogger()->warn(__VA_ARGS__)
//#define ICP_CORE_FATAL(...) ::Inception::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define ICP_ERROR(...) ::Inception::Log::GetClientLogger()->error(__VA_ARGS__)
#define ICP_INFO(...) ::Inception::Log::GetClientLogger()->info(__VA_ARGS__)
#define ICP_TRACE(...) ::Inception::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ICP_WARN(...) ::Inception::Log::GetClientLogger()->warn(__VA_ARGS__)
//#define ICP_FATAL(...) ::Inception::Log::GetClientLogger()->fatal(__VA_ARGS__)


