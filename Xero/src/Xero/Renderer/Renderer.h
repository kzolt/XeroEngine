#pragma once

#include "RendererContext.h"

namespace Xero {

	struct RendererConfig
	{
		uint32_t FramesInFlight = 3;
	};

	class Renderer
	{
	public:
		static RendererConfig& GetConfig();

		static uint32_t GetCurrentFrameIndex();
	};

}