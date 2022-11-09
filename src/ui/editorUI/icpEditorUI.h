#pragma once

#include "../../core/icpMacros.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

INCEPTION_BEGIN_NAMESPACE

class icpEditorUI
{
public:
	icpEditorUI();
	~icpEditorUI();

	void showEditorUI();

private:

	void showEntityHierarchy();
};

INCEPTION_END_NAMESPACE
