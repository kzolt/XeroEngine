#include "xopch.h"
#include "ImGuiLayer.h"

#include "Xero/Platform/Vulkan/VulkanImGuiLayer.h"

namespace Xero {

	// TODO: Create RendererAPIType Check
	ImGuiLayer* ImGuiLayer::Create()
	{
		return new VulkanImGuiLayer();
	}

}