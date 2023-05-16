#include "xopch.h"
#include "Renderer.h"

#include "Xero/Platform/Vulkan/VulkanSwapchain.h"

namespace Xero {

	RendererConfig& Renderer::GetConfig()
	{
		static RendererConfig config;
		return config;
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetWindow().GetSwapchain().GetCurrentBufferIndex();
	}

}