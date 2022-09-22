#include "icpRenderPassManager.h"
#include "icpVulkanRHI.h"
#include "icpVulkanUtility.h"
#include "../core/icpSystemContainer.h"
#include "../mesh/icpMeshResource.h"
#include "../resource/icpResourceSystem.h"

#include <vulkan/vulkan.hpp>

#include "renderPass/icpRenderPassBase.h"


INCEPTION_BEGIN_NAMESPACE
	icpRenderPassManager::icpRenderPassManager()
{
}


icpRenderPassManager::~icpRenderPassManager()
{
	cleanup();
}

bool icpRenderPassManager::initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI)
{
	m_rhi = vulkanRHI;

	icpRenderPassBase::RendePassInitInfo mainPassCreateInfo;
	mainPassCreateInfo.rhi = m_rhi;
	mainPassCreateInfo.passType = eRenderPass::MAIN_FORWARD_PASS;
	auto mainForwordPass = std::make_shared<icpRenderPassBase>();
	mainForwordPass->initializeRenderPass(mainPassCreateInfo);

	m_renderPasses.emplace_back(mainForwordPass);

	icpRenderPassBase::RendePassInitInfo uiPassInfo;
	uiPassInfo.rhi = m_rhi;
	uiPassInfo.passType = eRenderPass::UI_PASS;
	auto uiPass = std::make_shared<icpRenderPassBase>();
	uiPass->initializeRenderPass(mainPassCreateInfo);

	m_renderPasses.emplace_back(uiPass);

	return true;
}

void icpRenderPassManager::cleanup()
{
	for (const auto renderPass: m_renderPasses)
	{
		renderPass->cleanup();
	}
}

void icpRenderPassManager::render()
{
	for(const auto renderPass : m_renderPasses)
	{
		renderPass->render();
	}
}



INCEPTION_END_NAMESPACE