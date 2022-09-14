#pragma once
#include "../core/icpMacros.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

INCEPTION_BEGIN_NAMESPACE

enum class eEditorCommand : unsigned int
{
	CAMERA_LEFT = 1 << 0,  // A
	CMAERA_BACK = 1 << 1,  // S
	CAMERA_FORWARD = 1 << 2,  // W
	CAMERA_RIGHT = 1 << 3,  // D
	CAMERA_UP = 1 << 4,  // Q
	CAMERA_DOWN = 1 << 5,  // E
};

class icpWindowSystem
{
public:
	icpWindowSystem();
	~icpWindowSystem();

	bool initializeWindowSystem();
	GLFWwindow* getWindow() const;
	bool shouldClose() const;
	void pollEvent() const;

	static void onKeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void onMouseButtonCallBack(GLFWwindow* window, int button, int action, int mods);
	static void onCursorPosCallBack(GLFWwindow* window, double xpos, double ypos);

	unsigned int m_command = 0;

	bool m_mouseRightButtonDown = false;

	std::array<double, 2> m_mouseCoordBefore{ 0.0, 0.0 };
	std::array<double, 2> m_mouseCurCoord{ 0.0, 0.0 };

	glm::qua<float> m_originCameraRot;

	void tickWindow();
	void handleKeyEvent();
	void handleCursorMovement();
private:

	GLFWwindow* m_window = nullptr;
	int m_width = 0;
	int m_height = 0;
};



INCEPTION_END_NAMESPACE