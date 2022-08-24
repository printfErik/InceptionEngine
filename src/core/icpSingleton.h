#pragma once
#include "icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

template<class T>
class icpSingletonContainer
{
private:
	T* m_instance;
};


INCEPTION_END_NAMESPACE