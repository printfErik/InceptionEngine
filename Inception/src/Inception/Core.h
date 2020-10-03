#pragma once
#ifdef INCEPTION_PLATFORM_WINDOWS
	#ifdef INCEPTION_BUILD_DLL
		#define INCEPTION_API __declspec(dllexport)
	#else
		#define INCEPTION_API __declspec(dllimport)
	#endif 
#else
	#error	ONLY WINDOWS RIGHT NOW
#endif // INCEPTION_PLATFORM_WINDOWS

#ifdef INCEPTION_ENABLE_ASSERTS
	#define ICP_ASSERT(x,...) { if(!(x)} { ICP_ERROR("Assertion Failed: {0}", __VA__ARGS__); __debugbreak();} }
	#define ICP_CORE_ASSERT(x,...) { if(!(x)} { ICP_CORE_ERROR("Assertion Failed: {0}", __VA__ARGS__); __debugbreak();} }
#else
	#define ICP_ASSERT(x,...)
	#define ICP_CORE_ASSERT(x,...)
#endif // INCEPTION_ENABLE_ASSERTS


#define BIT(x) (1 << x)
