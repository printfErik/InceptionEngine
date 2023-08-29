#include "icpRenderSystem.h"
#include "RHI/Vulkan/icpVkGPUDevice.h"
#include "../core/icpSystemContainer.h"
#include "../scene/icpSceneSystem.h"
#include "../scene/icpXFormComponent.h"
#include "../mesh/icpPrimitiveRendererComponent.h"
#include "light/icpLightComponent.h"
#include "material/icpTextureRenderResourceManager.h"

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
	m_rhi = std::make_shared<icpVkGPUDevice>();
	m_rhi->initialize(g_system_container.m_windowSystem);

	auto vulkanRHI = std::dynamic_pointer_cast<icpVkGPUDevice>(m_rhi);
	m_renderPassManager = std::make_shared<icpRenderPassManager>();
	m_renderPassManager->initialize(vulkanRHI);

	m_textureRenderResourceManager = std::make_shared<icpTextureRenderResourceManager>(vulkanRHI);

	m_materialSystem = std::make_shared<icpMaterialSubSystem>();
	m_materialSystem->initializeMaterialSubSystem();

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
	m_textureRenderResourceManager->checkAndcleanAllDiscardedRenderResources();
}

void icpRenderSystem::setFrameBufferResized(bool _isResized)
{
	std::dynamic_pointer_cast<icpVkGPUDevice>(m_rhi)->m_framebufferResized = _isResized;
}

void icpRenderSystem::getAllStaticMeshRenderers()
{
	
}


void icpRenderSystem::drawCube()
{
	auto cubeEntity = g_system_container.m_sceneSystem->CreateEntity("Cube", nullptr);

	auto& primitive = cubeEntity->installComponent<icpPrimitiveRendererComponent>();

	// todo: implicit doing following steps
	primitive.m_primitive = ePrimitiveType::CUBE;

	primitive.FillInPrimitiveData(glm::vec3(1,0,1));
	primitive.CreateVertexBuffers();
	primitive.CreateIndexBuffers();
	primitive.CreateUniformBuffers();

	primitive.AllocateDescriptorSets();
}

void icpRenderSystem::createDirLight()
{
	auto dirLightEntity = g_system_container.m_sceneSystem->CreateEntity("DirLight", nullptr);

	auto& light = dirLightEntity->installComponent<icpDirectionalLightComponent>();
	light.m_type = eLightType::DIRECTIONAL_LIGHT;

	light.m_direction = glm::vec3(-1, -1, 1);
	light.m_color = glm::vec3(1, 1, 1);
}

std::shared_ptr<icpGPUDevice> icpRenderSystem::GetGPUDevice()
{
	return m_pGPUDevice;
}

std::shared_ptr<icpRenderPassManager> icpRenderSystem::GetRenderPassManager()
{
	return m_pRenderPassManager;
}

std::shared_ptr<icpMaterialSubSystem> icpRenderSystem::GetMaterialSubSystem()
{
	return m_materialSystem;
}


INCEPTION_END_NAMESPACE