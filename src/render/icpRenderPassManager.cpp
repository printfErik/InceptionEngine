#include "icpRenderPassManager.h"
#include "icpVulkanRHI.h"
#include "icpVulkanUtility.h"
#include "renderPass/icpMainForwardPass.h"
#include "renderPass/icpUiPass.h"

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
	std::shared_ptr<icpRenderPassBase> mainForwordPass = std::make_shared<icpMainForwardPass>();
	mainForwordPass->initializeRenderPass(mainPassCreateInfo);

	m_renderPasses.push_back(mainForwordPass);

	icpRenderPassBase::RendePassInitInfo uiPassInfo;
	uiPassInfo.rhi = m_rhi;
	uiPassInfo.passType = eRenderPass::UI_PASS;
	uiPassInfo.dependency = m_renderPasses[0];
	std::shared_ptr<icpRenderPassBase> uiPass = std::make_shared<icpUiPass>();
	uiPass->initializeRenderPass(uiPassInfo);

	m_renderPasses.push_back(uiPass);

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