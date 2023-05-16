#pragma once

#include "Xero/Core/Application.h"
#include "Xero/Renderer/RendererContext.h"

namespace Xero {

	enum class RendererAPIType
	{
		None = 0,
		Vulkan
	};

	enum class PrimitiveType
	{
		None = 0,
		Triangles,
		Lines
	};

	struct RendererAPICapabilities
	{
		std::string Vendor;
		std::string Renderer;
		std::string Version;

		int MaxSamples = 0;
		float MaxAnistropy = 0.0f;
		int MaxTextureUnits = 0;
	};

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

	public:
		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api) { s_CurrentRendererAPI = api; }

		static Ref<RendererContext> GetContext()
		{
			return Application::Get().GetWindow().GetRendererContext();
		}

	private:
		static RendererAPIType s_CurrentRendererAPI;
	};

}