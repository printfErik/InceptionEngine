#include "icpWindowSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpWindowSystem::icpWindowSystem()
{
	
}

icpWindowSystem::~icpWindowSystem()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{

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

	m_window = glfwCreateWindow(m_width, m_height, "Inception Engine", nullptr, nullptr);
	if (!m_window)
	{
		throw std::runtime_error("glfwCreateWindow failed");
	}

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	return true;
}

GLFWwindow* icpWindowSystem::getWindow() const
{
	return m_window;
}

bool icpWindowSystem::shouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

void icpWindowSystem::pollEvent() const
{
	glfwPollEvents();
}



INCEPTION_END_NAMESPACE