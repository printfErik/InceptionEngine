#include "icpCameraSystem.h"
#include "../scene/icpSceneSystem.h"
#include "../core/icpSystemContainer.h"

INCEPTION_BEGIN_NAMESPACE

void icpCameraComponent::initializeCamera() // todo read from mapfile
{
	m_position = glm::vec3(0.f, 0.f, 5.f);
	m_viewDir = glm::vec3(0.f, 0.f, -1.f);
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
	auto cameraView = g_system_container.m_sceneSystem->m_registry.view<icpCameraComponent>();
	if (cameraView.empty())
	{
		std::shared_ptr<icpCameraComponent> firstCamera = std::make_shared<icpCameraComponent>();
		firstCamera->initializeCamera();
		m_cameras.push_back(firstCamera);
		return;
	}

	for (auto entity: cameraView)
	{
		auto& editorCamera = cameraView.get<icpCameraComponent>(entity);
		m_cameras.push_back(std::make_shared<icpCameraComponent>(editorCamera));
	}
}

std::shared_ptr<icpCameraComponent> icpCameraSystem::getCurrentCamera()
{
	return m_cameras[0];
}

void icpCameraSystem::moveCamera(std::shared_ptr<icpCameraComponent> camera, const glm::vec3& offset)
{
	camera->m_position += offset;
}

glm::mat4 icpCameraSystem::getCameraViewMatrix(std::shared_ptr<icpCameraComponent> camera)
{
	camera->m_upDir = glm::conjugate(camera->m_rotation) * glm::vec3(0, 1.f, 0.f);
	camera->m_viewDir = glm::conjugate(camera->m_rotation) * glm::vec3(0.f, 0.f, -1.f);
	camera->m_viewMatrix = glm::lookAt(camera->m_position, camera->m_position + camera->m_viewDir, camera->m_upDir);
	return camera->m_viewMatrix;
}

void icpCameraSystem::rotateCamera(std::shared_ptr<icpCameraComponent> camera, double relativeXpos, double relativeYpos)
{
	auto pitch = glm::qua<float>(glm::vec3(camera->m_cameraRotationSpeed * (float)relativeYpos, 0.f, 0.f));
	auto yaw = glm::qua<float>(glm::vec3(0.f, camera->m_cameraRotationSpeed * (float)relativeXpos, 0.f));

	camera->m_rotation = pitch * camera->m_rotation * yaw;
}

INCEPTION_END_NAMESPACE