#pragma once
#include "../core/icpMacros.h"
#include <vulkan/vulkan.hpp>
#include "RHI/Vulkan/icpVkGPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	EDITOR_UI_PASS,
	//COPY_PASS,
	RENDER_PASS_COUNT
};

class icpRenderPassBase;

class icpRenderPassManager
{
public:
	icpRenderPassManager();
	~icpRenderPassManager();

	bool initialize(std::shared_ptr<icpGPUDevice> vulkanRHI);
	void cleanup();

	void render();

	std::shared_ptr<icpRenderPassBase> accessRenderPass(eRenderPass passType);

private:

	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;

	std::vector<std::shared_ptr<icpRenderPassBase>> m_renderPasses;

	uint32_t m_currentFrame = 0;
};

INCEPTION_END_NAMESPACE