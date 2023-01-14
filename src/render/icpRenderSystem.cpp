#include "icpRenderSystem.h"
#include "icpVulkanRHI.h"
#include "../core/icpSystemContainer.h"
#include "../scene/icpSceneSystem.h"
#include "../scene/icpXFormComponent.h"
#include "../mesh/icpPrimitiveRendererComponment.h"
#include "light/icpLightComponent.h"

INCEPTION_BEGIN_NAMESPACE

icpRenderSystem::icpRenderSystem()
{
}

icpRenderSystem::~icpRenderSystem()
{
	m_renderPassManager.reset();
	m_rhi.reset();
}

bool icpRenderSystem::initializeRenderSystem()
{
	m_rhi = std::make_shared<icpVulkanRHI>();
	m_rhi->initialize(g_system_container.m_windowSystem);

	m_renderPassManager = std::make_shared<icpRenderPassManager>();
	m_renderPassManager->initialize(std::dynamic_pointer_cast<icpVulkanRHI>(m_rhi));

	return true;
}
// example for typical loops in rendering
/*
for each view{
  bind view resources          // camera, environment...
  for each shader {
	bind shader pipeline
	bind shader resources      // shader control values
	for each material {
	  bind material resources  // material parameters and textures
	  for each object {
		bind object resources  // object transforms
		draw object
	  }
	}
  }
}
*/
void icpRenderSystem::drawFrame()
{
	m_renderPassManager->render();
}

void icpRenderSystem::setFrameBufferResized(bool _isResized)
{
	std::dynamic_pointer_cast<icpVulkanRHI>(m_rhi)->m_framebufferResized = _isResized;
}

void icpRenderSystem::getAllStaticMeshRenderers()
{
	
}

void icpRenderSystem::drawCube()
{
	auto cubeEntity = g_system_container.m_sceneSystem->createEntity("Cube", true);

	auto& primitive = cubeEntity.installComponent<icpPrimitiveRendererComponment>();

	primitive.m_primitive = ePrimitiveType::CUBE;

	primitive.createTextureImages();
	primitive.createTextureSampler();

	primitive.fillInPrimitiveData(glm::vec3(1,0,1));
	primitive.createVertexBuffers();
	primitive.createIndexBuffers();
	primitive.createUniformBuffers();

	primitive.allocateDescriptorSets();
}

void icpRenderSystem::createDirLight()
{
	auto dirLightEntity = g_system_container.m_sceneSystem->createEntity("DirLight", true);

	auto& light = dirLightEntity.installComponent<icpLightComponent>();
	light.m_type = eLightType::DIRECTIONAL_LIGHT;

	light.m_direction = glm::vec3(-1, -1, 1);
	light.m_ambient = glm::vec3(1, 0, 1);
	light.m_diffuse = glm::vec3(1, 1, 1);
	light.m_specular = glm::vec3(1, 1, 1);
}




INCEPTION_END_NAMESPACE