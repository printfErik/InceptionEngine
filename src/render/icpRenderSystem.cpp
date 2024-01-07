#include "icpRenderSystem.h"

#include "icpDeferredRenderer.h"
#include "icpForwardSceneRenderer.h"
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
	m_pSceneRenderer.reset();
	m_pGPUDevice.reset();
}

bool icpRenderSystem::initializeRenderSystem()
{
	m_pGPUDevice = std::make_shared<icpVkGPUDevice>();
	m_pGPUDevice->Initialize(g_system_container.m_windowSystem);

	m_pSceneRenderer = std::make_shared<icpDeferredRenderer>();
	m_pSceneRenderer->Initialize(m_pGPUDevice);

	m_textureRenderResourceManager = std::make_shared<icpTextureRenderResourceManager>(m_pGPUDevice);
	m_textureRenderResourceManager->InitializeEmptyTexture();

	m_materialSystem = std::make_shared<icpMaterialSubSystem>();
	m_materialSystem->initializeMaterialSubSystem();

	return true;
}

void icpRenderSystem::BuildRendererCompRenderResources()
{
	/*
	auto view = g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent, icpXFormComponent>();
	for (auto& entity : view)
	{
		auto& meshRenderer = view.get<icpMeshRendererComponent>(entity);
		if (meshRenderer.m_state == eRendererState::LINKED)
		{
			meshRenderer.prepareRenderResourceForMesh();
		}

	}
	*/
}

void icpRenderSystem::drawFrame()
{
	m_textureRenderResourceManager->UpdateManager();
	m_materialSystem->PrepareMaterialRenderResources();
	m_pSceneRenderer->Render();
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

std::shared_ptr<icpSceneRenderer> icpRenderSystem::GetSceneRenderer()
{
	return m_pSceneRenderer;
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