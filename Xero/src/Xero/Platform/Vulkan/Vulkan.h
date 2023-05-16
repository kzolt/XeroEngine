#pragma once

#include "vulkan/vulkan.h"

#define VK_CHECK_RESULT(f)											             \
{																	             \
	VkResult res = (f);												             \
	if (res != VK_SUCCESS)											             \
	{																             \
		XO_CORE_ERROR("VkResult is '{0}' in {1}:{2}", res, __FILE__ , __LINE__); \
		XO_CORE_ASSERT(res == VK_SUCCESS);										 \
	}																			 \
}