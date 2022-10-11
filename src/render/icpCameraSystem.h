#pragma once

#include "../core/icpMacros.h"
#include "../scene/icpComponent.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpCameraComponent : public icpComponentBase
{
public:
	glm::vec3 m_clearColor;
	float m_fov = 0.f;
	float m_aspectRatio = 0.f;
	float m_near = 0.f;
	float m_far = 0.f;
	glm::vec3 m_position;
	glm::qua<float> m_rotation;
	glm::mat4 m_viewMatrix;
	float m_cameraSpeed;
	float m_cameraRotationSpeed;
	glm::vec3 m_viewDir;
	glm::vec3 m_upDir;

	void initializeCamera();
	
private:

};


class icpCameraSystem
{
public:

	icpCameraSystem();
	~icpCameraSystem();

	void initialize();
	std::shared_ptr<icpCameraComponent> getCurrentCamera();
	void moveCamera(std::shared_ptr<icpCameraComponent> camera, const glm::vec3& offset);
	void rotateCamera(std::shared_ptr<icpCameraComponent> camera, double relativeXpos, double relativeYpos);

	glm::mat4 getCameraViewMatrix(std::shared_ptr<icpCameraComponent> camera);


private:

	std::vector<std::shared_ptr<icpCameraComponent>> m_cameras;

};

INCEPTION_END_NAMESPACE
