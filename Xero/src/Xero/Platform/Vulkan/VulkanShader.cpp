#include "xopch.h"
#include "VulkanShader.h"

#include "Xero/Core/Hash.h"
#include "Xero/Platform/Vulkan/VulkanContext.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv-tools/libspirv.hpp>

#include <filesystem>

#define OPTIMIZE_COMPILER false

namespace Xero {

	//////////////////////////////////////////////////////////////////////////////////
	// Utilities
	//////////////////////////////////////////////////////////////////////////////////

	namespace Utils {

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "Resources/Cache/Shader/Vulkan";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static ShaderUniformType SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
		{
			switch (type.basetype)
			{
			case spirv_cross::SPIRType::Boolean:  return ShaderUniformType::Bool;
			case spirv_cross::SPIRType::Int:
				if (type.vecsize == 1)            return ShaderUniformType::Int;
				if (type.vecsize == 2)            return ShaderUniformType::IVec2;
				if (type.vecsize == 3)            return ShaderUniformType::IVec3;
				if (type.vecsize == 4)            return ShaderUniformType::IVec4;

			case spirv_cross::SPIRType::UInt:     return ShaderUniformType::UInt;
			case spirv_cross::SPIRType::Float:
				if (type.columns == 3)            return ShaderUniformType::Mat3;
				if (type.columns == 4)            return ShaderUniformType::Mat4;

				if (type.vecsize == 1)            return ShaderUniformType::Float;
				if (type.vecsize == 2)            return ShaderUniformType::Vec2;
				if (type.vecsize == 3)            return ShaderUniformType::Vec3;
				if (type.vecsize == 4)            return ShaderUniformType::Vec4;
				break;
			}
			XO_CORE_ASSERT(false, "Unknown type!");
			return ShaderUniformType::None;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Buffers
	//////////////////////////////////////////////////////////////////////////////////

	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VulkanShader::UniformBuffer*>> s_UniformBuffers; // set -> binding point -> buffer
	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VulkanShader::StorageBuffer*>> s_StorageBuffers; // set -> binding point -> buffer

	//////////////////////////////////////////////////////////////////////////////////
	// Shaders
	//////////////////////////////////////////////////////////////////////////////////

	VulkanShader::VulkanShader(const std::string& path, bool forceCompile)
		: m_AssetPath(path)
	{
		size_t found = path.find_last_of("/\\");
		m_Name = found != std::string::npos ? path.substr(found + 1) : path;
		found = m_Name.find_last_of(".");
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

		Reload(forceCompile);
	}

	VulkanShader::~VulkanShader()
	{

	}

	void VulkanShader::ClearUniformBuffers()
	{
		s_UniformBuffers.clear();
		s_StorageBuffers.clear();
	}

	static std::string ReadShaderFromFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
		}
		else
		{
			XO_CORE_ASSERT(false, "Could not load shader!");
		}
		in.close();
		return result;
	}

	void VulkanShader::Reload(bool forceCompile /*= false*/)
	{
		// Clear old shader
		m_ShaderDescriptorSets.clear();
		m_Resources.clear();
		m_PushConstantRanges.clear();
		m_PipelineShaderStageCreateInfos.clear();
		m_DescriptorSetLayouts.clear();
		m_ShaderSource.clear();
		m_Buffers.clear();
		m_TypeCounts.clear();

		Utils::CreateCacheDirectoryIfNeeded();

		// Vertex and Fragment for now
		std::string source = ReadShaderFromFile(m_AssetPath);

		// TODO: Save shader hashes so we know when to re-compile out-of-date shaders
		uint32_t hash = Hash::GenerateFNVHash(source.c_str());

		m_ShaderSource = PreProcess(source);
		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
		CompileOrGetVulkanBinary(shaderData, forceCompile);
		LoadAndCreateShaders(shaderData);
		ReflectAllShaderStages(shaderData);

		CreateDescriptors();
	}

	size_t VulkanShader::GetHash() const
	{
		return std::hash<std::string>{}(m_AssetPath);
	}

	void VulkanShader::LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		m_PipelineShaderStageCreateInfos.clear();
		for (auto [stage, data] : shaderData)
		{
			XO_CORE_ASSERT(data.size());
			VkShaderModuleCreateInfo moduleCreateInfo{};

			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.codeSize = data.size() * sizeof(uint32_t);
			moduleCreateInfo.pCode = data.data();

			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

			VkPipelineShaderStageCreateInfo& shaderStage = m_PipelineShaderStageCreateInfos.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = stage;
			shaderStage.module = shaderModule;
			shaderStage.pName = "main";
		}
	}

	void VulkanShader::ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		m_Resources.clear();

		for (auto [stage, data] : shaderData)
			Reflect(stage, data);
	}

	void VulkanShader::Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		XO_CORE_TRACE("===========================");
		XO_CORE_TRACE(" Vulkan Shader Reflection");
		XO_CORE_TRACE(" {0}", m_AssetPath);
		XO_CORE_TRACE("===========================");

		spirv_cross::Compiler compiler(shaderData);
		auto resources = compiler.get_shader_resources();

		XO_CORE_TRACE("Uniform Buffers");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			int memberCount = (uint32_t)bufferType.member_types.size();
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			if (s_UniformBuffers[descriptorSet].find(binding) == s_UniformBuffers[descriptorSet].end())
			{
				UniformBuffer* uniformBuffer = new UniformBuffer();
				uniformBuffer->BindingPoint = binding;
				uniformBuffer->Size = size;
				uniformBuffer->Name = name;
				uniformBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
				s_UniformBuffers.at(descriptorSet)[binding] = uniformBuffer;
			}
			else
			{
				UniformBuffer* uniformBuffer = s_UniformBuffers.at(descriptorSet)[binding];
				if (size > uniformBuffer->Size)
					uniformBuffer->Size = size;
			}

			shaderDescriptorSet.UniformBuffers[binding] = s_UniformBuffers.at(descriptorSet)[binding];

			XO_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
			XO_CORE_TRACE("  Member Count: {0}", memberCount);
			XO_CORE_TRACE("  Size: {0}", size);
			XO_CORE_TRACE("-------------------");
		}

		XO_CORE_TRACE("Storage Buffers");
		for (const auto& resource : resources.storage_buffers)
		{
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t memberCount = (uint32_t)bufferType.member_types.size();
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			if (s_StorageBuffers[descriptorSet].find(binding) == s_StorageBuffers[descriptorSet].end())
			{
				StorageBuffer* storageBuffer = new StorageBuffer();
				storageBuffer->BindingPoint = binding;
				storageBuffer->Size = size;
				storageBuffer->Name = name;
				storageBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
				s_StorageBuffers.at(descriptorSet)[binding] = storageBuffer;
			}
			else
			{
				StorageBuffer* storageBuffer = s_StorageBuffers.at(descriptorSet).at(binding);
				if (size > storageBuffer->Size)
					storageBuffer->Size = size;
			}

			shaderDescriptorSet.StorageBuffers[binding] = s_StorageBuffers.at(descriptorSet).at(binding);

			XO_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
			XO_CORE_TRACE("  Member Count: {0}", memberCount);
			XO_CORE_TRACE("  Size: {0}", size);
			XO_CORE_TRACE("-------------------");
		}

		XO_CORE_TRACE("Push Constant Buffers:");
		for (const auto& resource : resources.push_constant_buffers)
		{
			const auto& bufferName = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			auto bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
			uint32_t memberCount = uint32_t(bufferType.member_types.size());
			uint32_t bufferOffset = 0;
			if (m_PushConstantRanges.size())
				bufferOffset = m_PushConstantRanges.back().Offset + m_PushConstantRanges.back().Size;

			auto& pushConstantRange = m_PushConstantRanges.emplace_back();
			pushConstantRange.ShaderStage = shaderStage;
			pushConstantRange.Size = bufferSize - bufferOffset;
			pushConstantRange.Offset = bufferOffset;

			// Skip empty push constant buffers - these are for the renderer only
			if (bufferName.empty() || bufferName == "u_Renderer")
				continue;

			ShaderBuffer& buffer = m_Buffers[bufferName];
			buffer.Name = bufferName;
			buffer.Size = bufferSize - bufferOffset;

			XO_CORE_TRACE("  Name: {0}", bufferName);
			XO_CORE_TRACE("  Member Count: {0}", memberCount);
			XO_CORE_TRACE("  Size: {0}", bufferSize);

			for (uint32_t i = 0; i < memberCount; i++)
			{
				auto type = compiler.get_type(bufferType.member_types[i]);
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				auto size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, i);
				auto offset = compiler.type_struct_member_offset(bufferType, i) - bufferOffset;

				std::string uniformName = fmt::format("{}.{}", bufferName, memberName);
				buffer.Uniforms[uniformName] = ShaderUniform(uniformName, Utils::SPIRTypeToShaderUniformType(type), size, offset);
			}
		}

		XO_CORE_TRACE("Sampled Images:");
		for (const auto& resource : resources.sampled_images)
		{
			const auto& name = resource.name;
			auto& baseType = compiler.get_type(resource.base_type_id);
			auto& type = compiler.get_type(resource.type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t dimension = baseType.image.dim;
			uint32_t arraySize = type.array[0];
			if (arraySize == 0)
				arraySize = 1;
			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			auto& imageSampler = shaderDescriptorSet.ImageSamplers[binding];
			imageSampler.BindingPoint = binding;
			imageSampler.DescriptorSet = descriptorSet;
			imageSampler.Name = name;
			imageSampler.ShaderStage = shaderStage;
			imageSampler.ArraySize = arraySize;

			m_Resources[name] = ShaderResourceDeclaration(name, binding, 1);

			XO_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
		}

		XO_CORE_TRACE("Storage Images:");
		for (const auto& resource : resources.storage_images)
		{
			const auto& name = resource.name;
			auto& type = compiler.get_type(resource.base_type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t dimension = type.image.dim;

			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			auto& imageSampler = shaderDescriptorSet.StorageImages[binding];
			imageSampler.BindingPoint = binding;
			imageSampler.DescriptorSet = descriptorSet;
			imageSampler.Name = name;
			imageSampler.ShaderStage = shaderStage;

			m_Resources[name] = ShaderResourceDeclaration(name, binding, 1);

			XO_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
		}

		XO_CORE_TRACE("===========================");
	}

	void VulkanShader::CreateDescriptors()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		//////////////////////////////////////////////////////////////////////
		// Descriptor Pool
		//////////////////////////////////////////////////////////////////////

		m_TypeCounts.clear();
		for (uint32_t set = 0; set < m_ShaderDescriptorSets.size(); set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSets[set];

			if (shaderDescriptorSet.UniformBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.UniformBuffers.size());
			}
			if (shaderDescriptorSet.StorageBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.StorageBuffers.size());
			}
			if (shaderDescriptorSet.ImageSamplers.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.ImageSamplers.size());
			}
			if (shaderDescriptorSet.StorageImages.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.StorageImages.size());
			}

			#if 0
			// TODO: Move this to the centralized renderer
			VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = m_TypeCounts.size();
			descriptorPoolInfo.pPoolSizes = m_TypeCounts.data();
			descriptorPoolInfo.maxSets = 1;

			VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));
			#endif

			//////////////////////////////////////////////////////////////////////
			// Descriptor Set Layout
			//////////////////////////////////////////////////////////////////////


			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			for (auto& [binding, uniformBuffer] : shaderDescriptorSet.UniformBuffers)
			{
				VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = uniformBuffer->ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;
				layoutBinding.binding = binding;

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[uniformBuffer->Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType;
				set.descriptorCount = 1;
				set.dstBinding = layoutBinding.binding;
			}

			for (auto& [binding, strorageBuffer] : shaderDescriptorSet.StorageBuffers)
			{
				VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = strorageBuffer->ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;
				layoutBinding.binding = binding;

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[strorageBuffer->Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType;
				set.descriptorCount = 1;
				set.dstBinding = layoutBinding.binding;
			}

			for (auto& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
			{
				auto& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.descriptorCount = imageSampler.ArraySize;
				layoutBinding.stageFlags = imageSampler.ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;
				layoutBinding.binding = binding;

				XO_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present!");
				XO_CORE_ASSERT(shaderDescriptorSet.StorageBuffers.find(binding) == shaderDescriptorSet.StorageBuffers.end(), "Binding is already present!");

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[imageSampler.Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType;
				set.descriptorCount = imageSampler.ArraySize;
				set.dstBinding = layoutBinding.binding;
			}

			for (auto& [bindingAndSet, imageSampler] : shaderDescriptorSet.StorageImages)
			{
				auto& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = imageSampler.ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;

				uint32_t binding = bindingAndSet & 0xffffffff;
				//uint32_t descriptorSet = (bindingAndSet >> 32);
				layoutBinding.binding = binding;

				XO_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present!");
				XO_CORE_ASSERT(shaderDescriptorSet.StorageBuffers.find(binding) == shaderDescriptorSet.StorageBuffers.end(), "Binding is already present!");
				XO_CORE_ASSERT(shaderDescriptorSet.ImageSamplers.find(binding) == shaderDescriptorSet.ImageSamplers.end(), "Binding is already present!");

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[imageSampler.Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType;
				set.descriptorCount = 1;
				set.dstBinding = layoutBinding.binding;
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
			descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayout.pNext = nullptr;
			descriptorLayout.bindingCount = (uint32_t)(layoutBindings.size());
			descriptorLayout.pBindings = layoutBindings.data();

			XO_CORE_INFO("Creating descriptor set {0} with {1} ubo's, {2} ssbo's, {3} samplers and {4} storage images", set,
				shaderDescriptorSet.UniformBuffers.size(),
				shaderDescriptorSet.StorageBuffers.size(),
				shaderDescriptorSet.ImageSamplers.size(),
				shaderDescriptorSet.StorageImages.size());
			if (set >= m_DescriptorSetLayouts.size())
				m_DescriptorSetLayouts.resize((size_t)(set + 1));
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &m_DescriptorSetLayouts[set]));
		}
	}

	Xero::VulkanShader::ShaderMaterialDescriptorSet VulkanShader::CreateDescriptorSets(uint32_t set /*= 0*/)
	{
		ShaderMaterialDescriptorSet result;

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		XO_CORE_ASSERT(m_TypeCounts.find(set) != m_TypeCounts.end());

		// TODO: Move this to the centralized renderer
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = (uint32_t)m_TypeCounts.at(set).size();
		descriptorPoolInfo.pPoolSizes = m_TypeCounts.at(set).data();
		descriptorPoolInfo.maxSets = 1;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = result.Pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];

		result.DescriptorSets.emplace_back();
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, result.DescriptorSets.data()));
		return result;
	}

	Xero::VulkanShader::ShaderMaterialDescriptorSet VulkanShader::CreateDescriptorSets(uint32_t set, uint32_t numberOfSets)
	{
		ShaderMaterialDescriptorSet result;

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> poolSizes;
		for (uint32_t set = 0; set < m_ShaderDescriptorSets.size(); set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSets[set];
			if (!shaderDescriptorSet) // Empty descriptor set
				continue;

			if (shaderDescriptorSet.UniformBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.UniformBuffers.size() * numberOfSets;
			}
			if (shaderDescriptorSet.StorageBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageBuffers.size() * numberOfSets;
			}
			if (shaderDescriptorSet.ImageSamplers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				uint32_t descriptorSetCount = 0;
				for (auto&& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
					descriptorSetCount += imageSampler.ArraySize;

				typeCount.descriptorCount = descriptorSetCount * numberOfSets;
			}
			if (shaderDescriptorSet.StorageImages.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageImages.size() * numberOfSets;
			}

		}

		XO_CORE_ASSERT(poolSizes.find(set) != poolSizes.end());

		// TODO: Move this to the centralized renderer
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = (uint32_t)poolSizes.at(set).size();
		descriptorPoolInfo.pPoolSizes = poolSizes.at(set).data();
		descriptorPoolInfo.maxSets = numberOfSets;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

		result.DescriptorSets.resize(numberOfSets);

		for (uint32_t i = 0; i < numberOfSets; i++)
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = result.Pool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];

			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result.DescriptorSets[i]));
		}
		return result;
	}

	Xero::VulkanShader::ShaderMaterialDescriptorSet VulkanShader::AllocateDescriptorSet(uint32_t set /*= 0*/)
	{
		XO_CORE_ASSERT(set < m_DescriptorSetLayouts.size());
		ShaderMaterialDescriptorSet result;

		if (m_ShaderDescriptorSets.empty())
			return result;

		// TODO: Remove
		result.Pool = nullptr;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];

		//VkDescriptorSet descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
		//result.DescriptorSets.push_back(descriptorSet);

		return result;
	}

	const VkWriteDescriptorSet* VulkanShader::GetDescriptorSet(const std::string& name, uint32_t set /*= 0*/) const
	{
		XO_CORE_ASSERT(set < m_ShaderDescriptorSets.size());
		XO_CORE_ASSERT(m_ShaderDescriptorSets[set]);
		if (m_ShaderDescriptorSets[set].WriteDescriptorSets.find(name) == m_ShaderDescriptorSets[set].WriteDescriptorSets.end())
		{
			XO_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			return nullptr;
		}

		return &m_ShaderDescriptorSets.at(set).WriteDescriptorSets.at(name);
	}

	std::vector<VkDescriptorSetLayout> VulkanShader::GetAllDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayout> result;
		result.reserve(m_DescriptorSetLayouts.size());
		for (auto& layout : m_DescriptorSetLayouts)
			result.emplace_back(layout);

		return result;
	}

	static const char* VkShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:		return ".cached_vulkan.vert";
		case VK_SHADER_STAGE_FRAGMENT_BIT:		return ".cached_vulkan.frag";
		case VK_SHADER_STAGE_COMPUTE_BIT:		return ".cached_vulkan.comp";
		}
		XO_CORE_ASSERT(false);
		return "";
	}

	static shaderc_shader_kind VkShaderStageToShaderC(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:		return shaderc_vertex_shader;
		case VK_SHADER_STAGE_FRAGMENT_BIT:		return shaderc_fragment_shader;
		case VK_SHADER_STAGE_COMPUTE_BIT:		return shaderc_compute_shader;
		}
		XO_CORE_ASSERT(false);
		return (shaderc_shader_kind)0;
	}

	void VulkanShader::CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile)
	{
		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		for (auto [stage, source] : m_ShaderSource)
		{
			auto extension = VkShaderStageCachedFileExtension(stage);
			if (!forceCompile)
			{
				std::filesystem::path p = m_AssetPath;
				auto path = cacheDirectory / (p.filename().string() + extension);
				std::string cachedFilePath = path.string();

				FILE* f;
				errno_t err = fopen_s(&f, cachedFilePath.c_str(), "rb");
				if (!err)
				{
					fseek(f, 0, SEEK_END);
					uint64_t size = ftell(f);
					fseek(f, 0, SEEK_SET);
					outputBinary[stage] = std::vector<uint32_t>(size / sizeof(uint32_t));
					fread(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}

			if (outputBinary[stage].size() == 0)
			{
				// TODO: This is kinda sketch
				shaderc::Compiler compiler;
				shaderc::CompileOptions options;
				options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
				options.SetWarningsAsErrors();
				options.SetGenerateDebugInfo();

				#if OPTIMIZE_COMPILER
				options.SetOptimizationLevel(shaderc_optimization_level_performance);
				#endif

				// Compile Shader
				{
					auto& shaderSource = m_ShaderSource[stage];
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, VkShaderStageToShaderC(stage), m_AssetPath.c_str(), options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						XO_CORE_ERROR(module.GetErrorMessage());
						XO_CORE_ASSERT(false);
					}

					const uint8_t* begin = (const uint8_t*)module.cbegin();
					const uint8_t* end = (const uint8_t*)module.cend();
					const ptrdiff_t size = end - begin;

					outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
				}

				// Cache Compiled Shader
				{
					std::filesystem::path p = m_AssetPath;
					auto path = cacheDirectory / (p.filename().string() + extension);
					std::string cachedFilePath = path.string();

					FILE* f;
					fopen_s(&f, cachedFilePath.c_str(), "wb");
					fwrite(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}
		}
	}

	static VkShaderStageFlagBits ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")							return VK_SHADER_STAGE_VERTEX_BIT;
		if (type == "fragment" || type == "pixel")		return VK_SHADER_STAGE_FRAGMENT_BIT;
		if (type == "compute")							return VK_SHADER_STAGE_COMPUTE_BIT;

		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

	std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& source)
	{
		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			XO_CORE_ASSERT(eol != std::string::npos, "Syntax Error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			XO_CORE_ASSERT(type == "vertex" || type == "fragment" || type == "pixel" || type == "compute", "Invalid shader type specificed");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			auto shaderType = ShaderTypeFromString(type);
			shaderSources[shaderType] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void VulkanShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{

	}

}