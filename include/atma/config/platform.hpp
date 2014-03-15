//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_CONFIG_PLATFORM_HPP
#define ATMA_CONFIG_PLATFORM_HPP
//=====================================================================
	
	// at the moment, we only support win32 :P
	#if defined(_WIN32) || defined(WIN32)
	#	define ATMA_PLATFORM_WIN32
	#	include "windows.h"
	#	include "windowsx.h"
	#	include "xmmintrin.h"
	#	undef min
	#	undef max
	#endif
	
//=====================================================================
#endif // inclusion guard
//=====================================================================