#include "icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE

icpBufferWriteDS::icpBufferWriteDS(const icpBufferRenderResource& bufferRes,
	VkDescriptorType type,
	uint32_t dstBinding,
	uint64_t _range,
	uint64_t _offset)
{
	bufferInfo.buffer = bufferRes.buffer;
	bufferInfo.offset = _offset;
	bufferInfo.range = _range;

	bufferWriteDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	bufferWriteDS.dstSet = VK_NULL_HANDLE;
	bufferWriteDS.dstBinding = dstBinding;
	bufferWriteDS.dstArrayElement = 0;
	bufferWriteDS.descriptorType = type;
	bufferWriteDS.descriptorCount = 1;
	bufferWriteDS.pBufferInfo = &bufferInfo;
}

INCEPTION_END_NAMESPACE