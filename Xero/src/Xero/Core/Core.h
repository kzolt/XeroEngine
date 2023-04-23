#pragma once

#include <memory>
#include "Log.h"

#ifdef XO_DEBUG
	#if defined(XO_PLATFORM_WINDOWS)
		#define XO_DEBUGBREAK() __debugbreak()
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
#else
	#define XO_DEBUGBREAK()
#endif

// __VA_ARGS__ expansion
#define XO_EXPAND_VARGS(x) x

#include "Assert.h"

#define BIT(x) (1 << x)

#define XO_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

namespace Xero {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	using RendererID = uint32_t;
	using byte = uint8_t;
}

#include "Xero/Core/KeyCodes.h"
