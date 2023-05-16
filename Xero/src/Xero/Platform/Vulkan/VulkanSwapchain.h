#pragma once

#include "Xero/Core/Core.h"

#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"

struct GLFWwinow;

namespace Xero {

	class VulkanSwapchain
	{
	public:
		VulkanSwapchain() = default;
		
		void Init(VkInstance instance, const Ref<VulkanDevice>& device);
		void InitSurface(GLFWwindow* windowHandle);

		void Create(uint32_t* width, uint32_t* height, bool vsync = false);

		void OnResize(uint32_t width, uint32_t height);

		void BeginFrame();
		void Present();

	private:
		void FindImageFormatAndColorSpace();

	private:
		VkInstance m_Instance;
		Ref<VulkanDevice> m_Device;

		VkFormat m_ColorFormat;
		VkColorSpaceKHR m_ColorSpace;

		uint32_t m_QueueNodeIndex = UINT32_MAX;
		uint32_t m_Width = 0, m_Height = 0;

		VkSurfaceKHR m_Surface;

		friend class VulkanContext;
	};

}