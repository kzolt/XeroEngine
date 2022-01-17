#pragma once

namespace Xero {

	class Application 
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:

	};

	// To be defined in CLIENT
	Application* CreateApplication();

}