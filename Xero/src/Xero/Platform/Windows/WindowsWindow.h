#pragma once

#include "Xero/Core/Window.h"

class GLFWwindow;

namespace Xero {

	class WindowsWindow : public Window 
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		virtual void ProcessEvent() override;
		virtual void SwapBuffers() override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) override;
		virtual bool IsVSync() const override { return m_Data.VSync; }

		inline virtual void* GetNativeWindow() const override { return m_Window; }
		virtual Ref<RendererContext> GetRendererContext() override { return m_RendererContext; }
		//virtual VulkanSwapchain& GetSwapchain() override { return m_Swapchain; }

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;

		Ref<RendererContext> m_RendererContext;
	};
}
