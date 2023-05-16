#pragma once

#include "Xero/Renderer/RendererContext.h"
#include "Xero/Renderer/RendererAPI.h"

#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"

namespace Xero {

	class VulkanContext : public RendererContext
	{
	public:
		VulkanContext();
		virtual ~VulkanContext();

		virtual void Init() override;

		Ref<VulkanDevice> GetDevice() { return m_Device; }

	public:
		static VkInstance GetInstance() { return s_VulkanInstance; }

		static Ref<VulkanContext> Get() { return Ref<VulkanContext>(RendererAPI::GetContext()); }
		static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		// Devices
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref<VulkanDevice> m_Device;

		// Vulkan Instance
		static VkInstance s_VulkanInstance;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache;

	};

}