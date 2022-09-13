#include "icpCameraSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpCameraSystem::icpCameraSystem()
{

}

icpCameraSystem::~icpCameraSystem()
{

}

void icpCameraSystem::initialize()
{
	icpCamera camera;
	camera.position = glm::vec3(0.f);
	camera.viewMatrix = glm::lookAt(camera.position, glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 1.0f, 0.0f));


	m_cameras.push_back(camera);
}



INCEPTION_END_NAMESPACE