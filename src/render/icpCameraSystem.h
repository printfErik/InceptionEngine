#pragma once

#include "../core/icpMacros.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

INCEPTION_BEGIN_NAMESPACE

struct icpCamera
{
	glm::vec3 clearColor;
	float fov = 0.f;
	float aspectRatio = 0.f;
	float near = 0.1f;
	float far = 1000.f;
	glm::vec3 position;
	glm::qua<float> rotation;
	glm::mat4 viewMatrix;
};


class icpCameraSystem
{
public:

	icpCameraSystem();
	~icpCameraSystem();

	void initialize();
	//moveCamera();


	std::vector<icpCamera> m_cameras;
private:

	

};

INCEPTION_END_NAMESPACE
