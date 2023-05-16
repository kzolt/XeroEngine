#include "xopch.h"
#include "VulkanContext.h"

#include <GLFW/glfw3.h>

namespace Xero {

	VkInstance VulkanContext::s_VulkanInstance = nullptr;

	static bool s_Validation = true;

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		XO_CORE_WARN("VulkanDebugCallback:\n  Object Type: {0}\n  Message: {1}", objectType, pMessage);
		return VK_FALSE;
	}

	VulkanContext::VulkanContext()
	{

	}

	VulkanContext::~VulkanContext()
	{

	}

	void VulkanContext::Init()
	{
		//////////////////////////////////////////////////////////////////////////
		// Application Info
		//////////////////////////////////////////////////////////////////////////
		
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Xero Engine";
		appInfo.pEngineName = "Xero";
		appInfo.apiVersion = VK_API_VERSION_1_3;

		//////////////////////////////////////////////////////////////////////////
		// Extensions and Validation
		//////////////////////////////////////////////////////////////////////////

		#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

		if (s_Validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = NULL;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		if (s_Validation)
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

			bool validationLayerPresent = false;

			XO_CORE_TRACE("Vulkan Instance Layers: ");
			for (const VkLayerProperties& layer : instanceLayerProperties)
			{
				XO_CORE_TRACE("\t{0}", layer.layerName);
				if (strcmp(layer.layerName, validationLayerName) == 0)
				{
					validationLayerPresent = true;
					break;
				}
			}

			if (validationLayerPresent)
			{
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
				XO_CORE_ERROR("Validation layer VK_LAYER_LUNARG_standard_validation not present, validation is disabled");
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Instance and Surface
		//////////////////////////////////////////////////////////////////////////

		VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &s_VulkanInstance));

		if (s_Validation)
		{
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_VulkanInstance, "vkCreateDebugReportCallbackEXT");
			XO_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT debugCreateInfo = {};
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			debugCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debugCreateInfo.pfnCallback = VulkanDebugReportCallback;
			debugCreateInfo.pUserData = NULL;
			VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(s_VulkanInstance, &debugCreateInfo, nullptr, &m_DebugReportCallback));
		}

		m_PhysicalDevice = VulkanPhysicalDevice::Select();

		VkPhysicalDeviceFeatures enabledFeatures = {};
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		m_Device = Ref<VulkanDevice>::Create(m_PhysicalDevice, enabledFeatures);

		VulkanAllocator::Init(m_Device);

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}

}