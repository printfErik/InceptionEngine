#include "Inception/Log.h"
#include "icppch.h"
#include "WindowsWindow.h"

namespace Inception {
	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.WindowTitle = props.WindowTitle;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		//ICP_CORE_TRACE("Creating window {0} ({1}, {2})", props.WindowTitle, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			ICP_CORE_ASSERT(sucess, "Could not intializa GLFW!");

			s_GLFWInitialized = true;
		}


		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.WindowTitle.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

	}

	void WindowsWindow::Shutdown() {
		glfwDestroyWindow(m_Window);
	}

	void WindowsWindow::OnUpdate() {
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	void WindowsWindow::SetVSync(bool enabled) {
		if (enabled) glfwSwapInterval(1);
		else glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const {
		return m_Data.VSync;
	}





}
