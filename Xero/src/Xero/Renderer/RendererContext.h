#pragma once

#include "Xero/Core/Ref.h"

struct GLFWwindow;

namespace Xero {

	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Init() = 0;

		static Ref<RendererContext> Create();
	};

}