#pragma once
#include "icpMacros.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
INCEPTION_BEGIN_NAMESPACE

class icpWindowSystem
{
public:
	icpWindowSystem();
	~icpWindowSystem();

	bool initializeWindowSystem();
	GLFWwindow* getWindow() const;
	bool shouldClose() const;
	void pollEvent() const;

private:

	GLFWwindow* m_window = nullptr;
	int m_width = 0;
	int m_height = 0;

};



INCEPTION_END_NAMESPACE