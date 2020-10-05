#pragma once
#include "Core.h"
#include "Events/Event.h"
#include "Inception/LayerStack.h"
#include "Inception/Events/ApplicationEvent.h"
#include "Window.h"

namespace Inception {

	class INCEPTION_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverLayer(Layer* layer);

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;
	};

	// To be defined in Client
	Application* CreateApplication();

}


