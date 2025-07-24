#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

void icpRenderPassBase::AddRenderpassInputLayout(VkDescriptorSetLayout layout)
{
	dsLayouts.push_back(layout);
}

INCEPTION_END_NAMESPACE