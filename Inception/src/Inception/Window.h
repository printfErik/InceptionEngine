#pragma once
#include "icppch.h"


#include "Inception/Core.h"
#include "Inception/Events/Event.h"

namespace Inception {

	struct WindowProps
	{
		std::string WindowTitle;
		unsigned int Width;
		unsigned int Height;

		WindowProps(const std::string& title = "Inception Game Engine",
			unsigned int w = 1280,
			unsigned int h = 720)
			: WindowTitle(title), Width(w), Height(h) {}
	};

	class INCEPTION_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		static Window* Create(const WindowProps& props = WindowProps());
	};

}