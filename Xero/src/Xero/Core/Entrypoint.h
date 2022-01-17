#pragma once

#ifdef XO_PLATFORM_WINDOWS

extern Xero::Application* Xero::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Xero::CreateApplication();
	app->Run();
	delete app;
}

#else
	#error Xero only supports windows
#endif