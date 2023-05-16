#pragma once

#include "Xero/Core/Layer.h"

namespace Xero {

	class ImGuiLayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		static ImGuiLayer* Create();
	};

}