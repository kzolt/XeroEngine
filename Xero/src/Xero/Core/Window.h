#pragma once

#include "xopch.h"

#include "Xero/Core/Core.h"
#include "Xero/Events/Event.h"
#include "Xero/Renderer/RendererContext.h"

namespace Xero {

	class VulkanSwapchain;

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool VSync;

		WindowProps(const std::string& title = "Xero Engine", uint32_t width = 1280, uint32_t height = 720)
			:Title(title), Width(width), Height(height), VSync(true) {}
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		virtual void ProcessEvent() = 0;
		virtual void SwapBuffers() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		virtual Ref<RendererContext> GetRendererContext() = 0;
		//virtual VulkanSwapchain& GetSwapchain() = 0;

		static Scope<Window> Create(const WindowProps& = WindowProps());
	};

}