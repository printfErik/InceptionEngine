#include "icpCameraSystem.h"

INCEPTION_BEGIN_NAMESPACE

void icpCameraComponent::initializeCamera()
{
	m_position = glm::vec3(0.f, 0.f, -5.f);
	m_viewDir = glm::vec3(0.f, 0.f, 1.f);
	m_upDir = glm::vec3(0.0f, 1.0f, 0.0f);
	m_viewMatrix = glm::lookAt(m_position, m_position + m_viewDir, m_upDir);
	m_fov = glm::radians(45.f);
	m_near = 0.1f;
	m_far = 100.f;
	m_cameraSpeed = 0.001f;
	m_cameraRotationSpeed = glm::radians(0.1f);
	m_rotation = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
}

icpCameraSystem::icpCameraSystem()
{

}

icpCameraSystem::~icpCameraSystem()
{

}

void icpCameraSystem::initialize()
{
	std::shared_ptr<icpCameraComponent> firstCamera = std::make_shared<icpCameraComponent>();
	firstCamera->initializeCamera();
	m_cameras.push_back(firstCamera);
}

std::shared_ptr<icpCameraComponent> icpCameraSystem::getCurrentCamera()
{
	return m_cameras[0];
}

void icpCameraSystem::moveCamera(std::shared_ptr<icpCameraComponent> camera, const glm::vec3& offset)
{
	camera->m_position += offset;
	updateCameraViewMatrix(camera);
}

void icpCameraSystem::updateCameraViewMatrix(std::shared_ptr<icpCameraComponent> camera)
{
	camera->m_viewMatrix = glm::lookAt(camera->m_position, camera->m_position + camera->m_viewDir, camera->m_upDir);
}

void icpCameraSystem::rotateCamera(std::shared_ptr<icpCameraComponent> camera, double relativeXpos, double relativeYpos, const glm::qua<float>& oriQua)
{
	camera->m_rotation = glm::rotate(oriQua, camera->m_cameraRotationSpeed * (float)relativeXpos, glm::vec3(0.f, -1.f, 0.f));
	camera->m_rotation = glm::rotate(camera->m_rotation, camera->m_cameraRotationSpeed * (float)relativeYpos, glm::vec3(1.f, 0.f, 0.f));

	auto inverse = glm::conjugate(camera->m_rotation);

	camera->m_viewDir = inverse * glm::vec3(0, 0, 1.f);
	updateCameraViewMatrix(camera);
}


INCEPTION_END_NAMESPACE