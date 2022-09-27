#include "icpWindowSystem.h"
#include "../core/icpSystemContainer.h"
#include "icpRenderSystem.h"
#include "icpCameraSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpWindowSystem::icpWindowSystem()
{
	
}

icpWindowSystem::~icpWindowSystem()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void icpWindowSystem::onKeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	icpWindowSystem* window_system = (icpWindowSystem*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS && window_system->m_mouseRightButtonDown == true)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			window_system->m_command |= (unsigned int)eEditorCommand::CAMERA_FORWARD;
			break;
		case GLFW_KEY_A:
			window_system->m_command |= (unsigned int)eEditorCommand::CAMERA_LEFT;
			break;
		case GLFW_KEY_S:
			window_system->m_command |= (unsigned int)eEditorCommand::CMAERA_BACK;
			break;
		case GLFW_KEY_D:
			window_system->m_command |= (unsigned int)eEditorCommand::CAMERA_RIGHT;
			break;
		case GLFW_KEY_Q:
			window_system->m_command |= (unsigned int)eEditorCommand::CAMERA_UP;
			break;
		case GLFW_KEY_E:
			window_system->m_command |= (unsigned int)eEditorCommand::CAMERA_DOWN;
			break;
		default:
			break;
		}
	}

	if (action == GLFW_RELEASE)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			window_system->m_command &= (0xFFFFFFFF ^ (unsigned int)eEditorCommand::CAMERA_FORWARD);
			break;
		case GLFW_KEY_A:
			window_system->m_command &= (0xFFFFFFFF ^ (unsigned int)eEditorCommand::CAMERA_LEFT);
			break;
		case GLFW_KEY_S:
			window_system->m_command &= (0xFFFFFFFF ^ (unsigned int)eEditorCommand::CMAERA_BACK);
			break;
		case GLFW_KEY_D:
			window_system->m_command &= (0xFFFFFFFF ^ (unsigned int)eEditorCommand::CAMERA_RIGHT);
			break;
		case GLFW_KEY_Q:
			window_system->m_command &= (0xFFFFFFFF ^ (unsigned int)eEditorCommand::CAMERA_UP);
			break;
		case GLFW_KEY_E:
			window_system->m_command &= (0xFFFFFFFF ^ (unsigned int)eEditorCommand::CAMERA_DOWN);
			break;
		default:
			break;
		}
	}
}

void icpWindowSystem::onMouseButtonCallBack(GLFWwindow* window, int button, int action, int mods)
{
	icpWindowSystem* window_system = (icpWindowSystem*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS)
	{
		switch (button)
		{
		case GLFW_MOUSE_BUTTON_RIGHT:
		{
			window_system->m_mouseRightButtonDown = true;
			break;
		}
		default:
			break;
		}
	}

	if (action == GLFW_RELEASE)
	{
		switch (button)
		{
		case GLFW_MOUSE_BUTTON_RIGHT:
		{
			window_system->m_mouseRightButtonDown = false;
			break;
		}
		default:
			break;
		}
	}
}

void icpWindowSystem::onCursorPosCallBack(GLFWwindow* window, double xpos, double ypos)
{
	icpWindowSystem* window_system = (icpWindowSystem*)glfwGetWindowUserPointer(window);

	if (window_system->m_mouseCurCoord[0] >= 0.0f && window_system->m_mouseCurCoord[1] >= 0.0f)
	{
		if (window_system->m_mouseRightButtonDown)
		{
			auto relativeXpos = xpos - window_system->m_mouseCurCoord[0];
			auto relativeYpos = ypos - window_system->m_mouseCurCoord[1];
			auto camera = g_system_container.m_cameraSystem->getCurrentCamera();
			g_system_container.m_cameraSystem->rotateCamera(camera, relativeXpos, relativeYpos);
		}
	}

	window_system->m_mouseCurCoord[0] = xpos;
	window_system->m_mouseCurCoord[1] = ypos;
}


bool icpWindowSystem::initializeWindowSystem()
{
	if (glfwInit() != GLFW_TRUE)
	{
		throw std::runtime_error("glfwInit failed");
	}

	m_width = 500;
	m_height = 500;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(m_width, m_height, "Inception Engine", nullptr, nullptr);
	if (!m_window)
	{
		throw std::runtime_error("glfwCreateWindow failed");
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, onKeyCallBack);
	glfwSetMouseButtonCallback(m_window, onMouseButtonCallBack);
	glfwSetCursorPosCallback(m_window, onCursorPosCallBack);

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

void icpWindowSystem::tickWindow()
{
	pollEvent();

	handleKeyEvent();
}

void icpWindowSystem::handleKeyEvent()
{
	auto camera = g_system_container.m_cameraSystem->getCurrentCamera();
	auto cameraRotation = glm::conjugate(camera->m_rotation) ;
	glm::vec3 cameraOffset = glm::vec3(0.f);

	if(m_command & (unsigned int)eEditorCommand::CAMERA_FORWARD)
	{
		cameraOffset -= cameraRotation * glm::vec3(0.f, 0.f, camera->m_cameraSpeed);
	}
	if (m_command & (unsigned int)eEditorCommand::CMAERA_BACK)
	{
		cameraOffset += cameraRotation * glm::vec3(0.f, 0.f, camera->m_cameraSpeed);
	}
	if (m_command & (unsigned int)eEditorCommand::CAMERA_RIGHT)
	{
		cameraOffset += cameraRotation * glm::vec3(camera->m_cameraSpeed, 0.f, 0.f);
	}
	if (m_command & (unsigned int)eEditorCommand::CAMERA_LEFT)
	{
		cameraOffset -= cameraRotation * glm::vec3(camera->m_cameraSpeed, 0.f, 0.f);
	}
	if (m_command & (unsigned int)eEditorCommand::CAMERA_UP)
	{
		cameraOffset += cameraRotation * glm::vec3(0.f, camera->m_cameraSpeed, 0.f);
	}
	if (m_command & (unsigned int)eEditorCommand::CAMERA_DOWN)
	{
		cameraOffset += cameraRotation * glm::vec3(0.f, -camera->m_cameraSpeed, 0.f);
	}

	if (cameraOffset != glm::vec3(0.f))
		g_system_container.m_cameraSystem->moveCamera(camera, cameraOffset);
}



INCEPTION_END_NAMESPACE