#pragma once
#include "Core.h"

namespace Inception {

	class INCEPTION_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// To be defined in Client
	Application* CreateApplication();

}


