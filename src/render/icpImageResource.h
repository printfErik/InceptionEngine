#pragma once
#include "../core/icpMacros.h"
#include "../resource/icpResourceBase.h"


INCEPTION_BEGIN_NAMESPACE
class icpImageResource : public icpResourceBase
{
public:
	icpImageResource();
	~icpImageResource();

	void setImageBuffer(unsigned char* imgBuffer, size_t size, size_t width, size_t height, size_t channelNum);
	std::vector<unsigned char>& getImgBuffer();

	size_t m_imgWidth = 0;
	size_t m_height = 0;
	size_t m_channelNum = 0;

private:

	std::vector<unsigned char> m_imgBuffer;
};


INCEPTION_END_NAMESPACE