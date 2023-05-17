#include "EditorLayer.h"

namespace Xero {

	EditorLayer::EditorLayer()
		: Layer("Editor Layer")
	{

	}

	EditorLayer::~EditorLayer()
	{

	}

	void EditorLayer::OnAttach()
	{

	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnUpdate()
	{

	}

	void EditorLayer::OnImGuiRender()
	{
		bool show = true;
		ImGui::ShowDemoWindow(&show);
	}

	void EditorLayer::OnEvent(Event& event)
	{

	}

}