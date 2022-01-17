#include <Xero.h>

class Sandbox : public Xero::Application
{
public:
	Sandbox() {}
	~Sandbox() {}

};

Xero::Application* Xero::CreateApplication()
{
	return new Sandbox();
}