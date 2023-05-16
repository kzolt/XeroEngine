#include "xopch.h"
#include "VulkanDevice.h"

#include "VulkanContext.h"

namespace Xero {

	//////////////////////////////////////////////////////////////////////////
	// Vulkan Physical Device
	//////////////////////////////////////////////////////////////////////////

	VulkanPhysicalDevice::VulkanPhysicalDevice()
	{
		auto vkInstance = VulkanContext::GetInstance();

		// Get number of available physical devices
		uint32_t gpuCount = 0;
		vkEnumeratePhysicalDevices(vkInstance, &gpuCount, nullptr);
		XO_CORE_ASSERT(gpuCount > 0); 

		// Enumerate Devices
		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &gpuCount, physicalDevices.data()));

		VkPhysicalDevice selectedPhysicalDevice = nullptr;
		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			vkGetPhysicalDeviceProperties(physicalDevice, &m_Properties);
			if (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				selectedPhysicalDevice = physicalDevice;
				break;
			}
		}

		if (!selectedPhysicalDevice)
		{
			XO_CORE_TRACE("Could not find discrete GPU.");
			selectedPhysicalDevice = physicalDevices.back();
		}

		XO_CORE_ASSERT(selectedPhysicalDevice, "Could not find any physical devices!");
		m_PhysicalDevice = selectedPhysicalDevice;

		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		XO_CORE_ASSERT(queueFamilyCount > 0);
		m_QueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				XO_CORE_TRACE("Selected physical device has {0} extensions", extensions.size());
				for (const auto& ext : extensions)
				{
					m_SupportedExtensions.emplace(ext.extensionName);
					XO_CORE_TRACE("\t{0}", ext.extensionName);
				}
			}
		}

		// Queue Family
		static const float defaultQueuePriority = 0.0f;

		int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		m_QueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);

		// Graphics Queue
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
		{
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			m_QueueCreateInfos.push_back(queueInfo);
		}

		// Dedicated compute queue
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
		{
			if (m_QueueFamilyIndices.Compute != m_QueueFamilyIndices.Graphics)
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				m_QueueCreateInfos.push_back(queueInfo);
			}
		}

		// Dedicated transfer queue
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
		{
			if ((m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Graphics) && (m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Compute))
			{
				// If transfer family index differs, we need an additional queue create info for the compute transfer queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				m_QueueCreateInfos.push_back(queueInfo);
			}
		}

		m_DepthFormat = FindDepthFormat();
		XO_CORE_ASSERT(m_DepthFormat);
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{

	}

	bool VulkanPhysicalDevice::IsExtensionSupported(const std::string& extensionName) const
	{
		return m_SupportedExtensions.find(extensionName) != m_SupportedExtensions.end();
	}

	uint32_t VulkanPhysicalDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
	{
		for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					return i;
			}
			typeBits >> 1;
		}

		XO_CORE_ASSERT(false, "Could not find a suitable memory type!");
		return UINT32_MAX;
	}

	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Select()
	{
		return Ref<VulkanPhysicalDevice>::Create();
	}

	VkFormat VulkanPhysicalDevice::FindDepthFormat() const
	{
		// Since all depth formats may be optimal, we need to find a suitable depth format to use
		// Star with the highest precision packed format
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);

			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				return format;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::GetQueueFamilyIndices(int flags)
	{
		QueueFamilyIndices indicies;

		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (flags & VK_QUEUE_COMPUTE_BIT)
		{
			for (size_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indicies.Compute = i;
					break;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics
		if (flags & VK_QUEUE_TRANSFER_BIT)
		{
			for (size_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indicies.Transfer = i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (size_t i = 0; i < m_QueueFamilyProperties.size(); i++)
		{
			if ((flags & VK_QUEUE_TRANSFER_BIT) && indicies.Transfer == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
					indicies.Transfer = i;
			}

			if ((flags & VK_QUEUE_COMPUTE_BIT) && indicies.Compute == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					indicies.Compute = i;
			}

			if ((flags & VK_QUEUE_GRAPHICS_BIT) && indicies.Graphics == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indicies.Graphics = i;
			}
		}

		return indicies;
	}

	//////////////////////////////////////////////////////////////////////////
	// Vulkan Device
	//////////////////////////////////////////////////////////////////////////

	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
		: m_PhysicalDevice(physicalDevice), m_EnabledFeatures(enabledFeatures)
	{
		std::vector<const char*> deviceExtensions;

		// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extensions
		XO_CORE_ASSERT(m_PhysicalDevice->IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->m_QueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = physicalDevice->m_QueueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		// If a pNext(chain) has been passed, we need to add it to the device creation info
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

		// Enable the debug marker extension if it's present
		if (m_PhysicalDevice->IsExtensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		{
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			m_EnabledDebugMarkers = true;
		}

		if (deviceExtensions.size() > 0)
		{
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &m_LogicalDevice));

		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_PhysicalDevice->m_QueueFamilyIndices.Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolInfo, nullptr, &m_CommandPool));

		// Get a graphics queue from the device
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
	}

	VulkanDevice::~VulkanDevice()
	{

	}

	VkCommandBuffer VulkanDevice::GetCommandBuffer(bool begin)
	{
		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = m_CommandPool;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufAllocInfo, &cmdBuffer));

		// If requested, also start the new command buffer
		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
		}

		return cmdBuffer;
	}

	void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer)
	{
		const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
		XO_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;

		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &fence));

		// Submit to the queue
		VK_CHECK_RESULT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(m_LogicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(m_LogicalDevice, fence, nullptr);
		vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);
	}

	VkCommandBuffer VulkanDevice::CreateSecondaryCommandBuffer()
	{
		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocInfo{};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = m_CommandPool;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdBufAllocInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufAllocInfo, &cmdBuffer));
		return cmdBuffer;
	}

}