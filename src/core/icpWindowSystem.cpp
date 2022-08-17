#include "icpWindowSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpWindowSystem::icpWindowSystem()
{
	
}

icpWindowSystem::~icpWindowSystem()
{
	glfwDestroyWindow(m_window.get());
	glfwTerminate();
}

bool icpWindowSystem::initializeWindowSystem()
{
	if (glfwInit() != GLFW_TRUE)
	{
		throw std::runtime_error("glfwInit failed");
	}

	m_width = 800;
	m_height = 600;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto window = glfwCreateWindow(m_width, m_height, "Inception Engine", nullptr, nullptr);
	//m_window = std::shared_ptr<GLFWwindow>(window);

	/*
	if (!m_window)
	{
		throw std::runtime_error("glfwCreateWindow failed");
	}
	*/
}

std::shared_ptr<GLFWwindow> icpWindowSystem::getWindow() const
{
	return m_window;
}


INCEPTION_END_NAMESPACE