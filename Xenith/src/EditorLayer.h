#pragma once

#include <Xero.h>

#include "Xero/ImGui/ImGuiLayer.h"
#include "ImGui/imgui_internal.h"

namespace Xero {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate() override;

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;
	};

}