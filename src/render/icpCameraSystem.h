#pragma once

#include "../core/icpMacros.h"
#include <glm/glm.hpp>
INCEPTION_BEGIN_NAMESPACE

struct icpCamera
{
	glm::vec3 clearColor;
	float near = 0.f;
	float far = 1000.f;

};


class icpCameraSystem
{
public:

	icpCameraSystem();
	~icpCameraSystem();

	initialize();
	moveCamera();

private:

	std::vector<icpCamera> m_cameras;

};

INCEPTION_END_NAMESPACE
