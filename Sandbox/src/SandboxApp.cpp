#include <Xero.h>

#include "imgui.h"

class ExampleLayer : public Xero::Layer
{
public:
	ExampleLayer()
		: Layer("Example") {}

	void OnUpdate() override
	{

	}

	void OnEvent(Xero::Event& e) override
	{

	}

	void OnImGuiRender() override
	{
		bool show = true;
		ImGui::ShowDemoWindow(&show);
	}
};

class Sandbox : public Xero::Application
{
public:
	Sandbox() 
	{
		PushLayer(new ExampleLayer());
	}
	
	~Sandbox() 
	{

	}

};

Xero::Application* Xero::CreateApplication()
{
	return new Sandbox();
}