#include "xopch.h"
#include "RendererContext.h"
#include "RendererAPI.h"

#include "Xero/Platform/Vulkan/VulkanContext.h"

namespace Xero {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::Vulkan:	return Ref<VulkanContext>::Create();
		}

		XO_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}