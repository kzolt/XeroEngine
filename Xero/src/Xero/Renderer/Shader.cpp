#include "xopch.h"
#include "Shader.h"

#include "RendererAPI.h"

#include "Xero/Platform/Vulkan/VulkanShader.h"

namespace Xero {

	Ref<Shader> Shader::Create(const std::string& filepath, bool forceCompile)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::Vulkan:		return Ref<VulkanShader>::Create(filepath, forceCompile);
		}

		XO_CORE_ASSERT(false, "Unknown RendererAPI");
	}

	ShaderLibrary::ShaderLibrary()
	{

	}

	ShaderLibrary::~ShaderLibrary()
	{

	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		XO_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(const std::string& path, bool forceCompile /*= false*/)
	{
		auto shader = Shader::Create(path, forceCompile);
		auto& name = shader->GetName();
		XO_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(const std::string& name, const std::string& path)
	{
		XO_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = Shader::Create(path);
	}

	const Ref<Shader>& ShaderLibrary::Get(const std::string& name) const
	{
		XO_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders.at(name);
	}

	ShaderUniform::ShaderUniform(const std::string& name, const ShaderUniformType type, uint32_t size, uint32_t offset)
		: m_Name(name), m_Type(type), m_Size(size), m_Offset(offset)
	{

	}

	const std::string& ShaderUniform::UniformTypeToString(const ShaderUniformType type)
	{
		switch (type)
		{
		case ShaderUniformType::Bool:	return std::string("Boolean");
		case ShaderUniformType::Int:	return std::string("Int");
		case ShaderUniformType::Float:	return std::string("Float");
		}

		return std::string("None");
	}

}