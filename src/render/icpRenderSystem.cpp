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
	m_pRenderPassManager.reset();
	m_pGPUDevice.reset();
}

bool icpRenderSystem::initializeRenderSystem()
{
	m_pGPUDevice = std::make_shared<icpVkGPUDevice>();
	m_pGPUDevice->Initialize(g_system_container.m_windowSystem);

	m_pRenderPassManager = std::make_shared<icpRenderPassManager>();
	m_pRenderPassManager->initialize(m_pGPUDevice);

	m_textureRenderResourceManager = std::make_shared<icpTextureRenderResourceManager>(m_pGPUDevice);
	m_textureRenderResourceManager->InitializeEmptyTexture();

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
	m_pRenderPassManager->render();
	m_textureRenderResourceManager->checkAndcleanAllDiscardedRenderResources();
}

void icpRenderSystem::setFrameBufferResized(bool _isResized)
{
	m_pGPUDevice->m_framebufferResized = _isResized;
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

std::shared_ptr<icpTextureRenderResourceManager> icpRenderSystem::GetTextureRenderResourceManager()
{
	return m_textureRenderResourceManager;
}


INCEPTION_END_NAMESPACE