#include "xopch.h"
#include "Application.h"
#include "Log.h"

namespace Xero {

	Application::Application()
	{
		Log::Init();
		XO_CORE_INFO("Log Initialized");
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{

	}

}