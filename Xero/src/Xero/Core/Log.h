#pragma once

#include <memory>

#include "spdlog/spdlog.h"

namespace Xero {

	class Log
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

// Core Log Macros
#define XO_CORE_TRACE(...)	::Xero::Log::GetCoreLogger()->trace(__VA_ARGS__);
#define XO_CORE_INFO(...)	::Xero::Log::GetCoreLogger()->info(__VA_ARGS__);
#define XO_CORE_WARN(...)	::Xero::Log::GetCoreLogger()->warn(__VA_ARGS__);
#define XO_CORE_ERROR(...)	::Xero::Log::GetCoreLogger()->error(__VA_ARGS__);
#define XO_CORE_FATAL(...)	::Xero::Log::GetCoreLogger()->critical(__VA_ARGS__);


// Client Log Macros
#define XO_TRACE(...)	::Xero::Log::GetClientLogger()->trace(__VA_ARGS__);
#define XO_INFO(...)	::Xero::Log::GetClientLogger()->info(__VA_ARGS__);
#define XO_WARN(...)	::Xero::Log::GetClientLogger()->warn(__VA_ARGS__);
#define XO_ERROR(...)	::Xero::Log::GetClientLogger()->error(__VA_ARGS__);
#define XO_FATAL(...)	::Xero::Log::GetClientLogger()->critical(__VA_ARGS__);