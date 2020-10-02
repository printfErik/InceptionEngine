#pragma once
#ifdef INCEPTION_PLATFORM_WINDOWS
	#ifdef INCEPTION_PLATFORM_WINDOWS
		#define INCEPTION_API __declspec(dllexport)
	#else
		#define INCEPTION_API __declspec(dllimport)
	#endif 
#else
	#error	ONLY WINDOWS!
#endif // INCEPTION_PLATFORM_WINDOWS
