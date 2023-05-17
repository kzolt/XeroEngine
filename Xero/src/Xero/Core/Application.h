#pragma once

#include "Xero/Core/Core.h"
#include "Xero/Core/Window.h"
#include "Xero/Core/LayerStack.h"

#include "Xero/Events/Event.h"
#include "Xero/Events/ApplicationEvent.h"
#include "Xero/ImGui/ImGuiLayer.h"

namespace Xero {

	class Application 
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

	public:
		inline static Application& Get() { return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		Scope<Window> m_Window;
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;

		bool m_Running = true;

	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}