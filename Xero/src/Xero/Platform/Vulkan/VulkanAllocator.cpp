#include "xopch.h"
#include "VulkanAllocator.h"

#include "VulkanContext.h"

#include "Xero/Utils/StringUtils.h"

namespace Xero {

	//////////////////////////////////////////////////////////////////////////
	// Helpers
	//////////////////////////////////////////////////////////////////////////

	struct VulkanAllocatorData
	{
		VmaAllocator Allocator;
		uint64_t TotalAllocatedBytes = 0;
	};

	static VulkanAllocatorData* s_Data = nullptr;

	//////////////////////////////////////////////////////////////////////////
	// Vulkan Memory Allocator
	//////////////////////////////////////////////////////////////////////////

	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_Tag(tag)
	{

	}

	VulkanAllocator::~VulkanAllocator()
	{

	}

	VmaAllocation VulkanAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_Data->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);

		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		XO_CORE_TRACE("Vulkan Allocator ({0}): allocating buffer; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			XO_CORE_TRACE("Vulkan Allocator ({0}): total allocated since start is = {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

		return allocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_Data->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);

		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		XO_CORE_TRACE("VulkanAllocator ({0}): allocating image; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			XO_CORE_TRACE("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}
		return allocation;
	}

	void VulkanAllocator::Free(VmaAllocation allocation)
	{
		vmaFreeMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		XO_CORE_ASSERT(image);
		XO_CORE_ASSERT(allocation);
		vmaDestroyImage(s_Data->Allocator, image, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		XO_CORE_ASSERT(buffer);
		XO_CORE_ASSERT(allocation);
		vmaDestroyBuffer(s_Data->Allocator, buffer, allocation);
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocator::DumpStats()
	{
		const auto& memoryProps = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryHeapCount);
		vmaGetHeapBudgets(s_Data->Allocator, budgets.data());

		XO_CORE_WARN("-----------------------------------");
		for (VmaBudget& b : budgets)
		{
			XO_CORE_WARN("VmaBudget.budget = {0}", Utils::BytesToString(b.budget));
			XO_CORE_WARN("VmaBudget.usage = {0}", Utils::BytesToString(b.usage));
		}
		XO_CORE_WARN("-----------------------------------");
	}

	GPUMemoryStats VulkanAllocator::GetStats()
	{
		const auto& memoryProps = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryHeapCount);
		vmaGetHeapBudgets(s_Data->Allocator, budgets.data());

		uint64_t usage = 0;
		uint64_t budget = 0;

		for (VmaBudget& b : budgets)
		{
			usage += b.usage;
			budget += b.budget;
		}

		return { usage, budget };
	}

	void VulkanAllocator::Init(Ref<VulkanDevice> device)
	{
		s_Data = new VulkanAllocatorData();

		// Initialize VulkanMemoryAllocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		allocatorInfo.device = device->GetVulkanDevice();
		allocatorInfo.instance = VulkanContext::GetInstance();

		vmaCreateAllocator(&allocatorInfo, &s_Data->Allocator);
	}

	void VulkanAllocator::Shutdown()
	{
		vmaDestroyAllocator(s_Data->Allocator);

		delete s_Data;
		s_Data = nullptr;
	}

	VmaAllocator& VulkanAllocator::GetVMAAllocator()
	{
		return s_Data->Allocator;
	}

}