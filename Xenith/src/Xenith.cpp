#include <Xero.h>
#include <Xero/Core/Entrypoint.h>

#include "EditorLayer.h"

class XenithApplication : public Xero::Application
{
public:
	XenithApplication()
	{
		PushLayer(new Xero::EditorLayer());
	}

	~XenithApplication()
	{

	}

};

Xero::Application* Xero::CreateApplication()
{
	return new XenithApplication();
}