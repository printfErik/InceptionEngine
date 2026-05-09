#pragma once

#include "../../core/icpMacros.h"

#include <backends/imgui_impl_glfw.h>
#if !defined(INCEPTION_RENDER_BACKEND_D3D12)
#include <backends/imgui_impl_vulkan.h>
#endif

INCEPTION_BEGIN_NAMESPACE

class icpGameEntity;

class icpEditorUI
{
public:
	icpEditorUI();
	~icpEditorUI();

	void showEditorUI();
	void showEditorDockingSpaceUI();
	void recursiveAddEntityToHierarchy(std::shared_ptr<icpGameEntity> entity);
	void drawCube();
	void createDirLight();

private:
	void showEntityRightClickMenu(const char* entityName);
	void showEntityHierarchy();
	void showFrameStatsOverlay();
};

INCEPTION_END_NAMESPACE
