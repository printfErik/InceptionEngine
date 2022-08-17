#pragma once
#include "icpMacros.h"
#include <iostream>

#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"
INCEPTION_BEGIN_NAMESPACE

class icpWindowSystem
{
public:
	icpWindowSystem();
	~icpWindowSystem();

	bool initializeWindowSystem();
	std::shared_ptr<GLFWwindow> getWindow() const;

private:

	std::shared_ptr<GLFWwindow> m_window = nullptr;
	int m_width = 0;
	int m_height = 0;

};



INCEPTION_END_NAMESPACE