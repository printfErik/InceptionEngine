#include "icpImageResource.h"

INCEPTION_BEGIN_NAMESPACE
icpImageResource::icpImageResource()
{
	
}

icpImageResource::~icpImageResource()
{
	
}

void icpImageResource::setImageBuffer(unsigned char* imgBuffer, size_t size, size_t width, size_t height, size_t channelNum)
{
	m_imgBuffer.resize(size);

	m_imgWidth = width;
	m_height = height;
	m_channelNum = channelNum;

	memcmp(m_imgBuffer.data(), imgBuffer, size);
}

std::vector<unsigned char>& icpImageResource::getImgBuffer()
{
	return m_imgBuffer;
}

INCEPTION_END_NAMESPACE