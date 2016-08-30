#pragma once

// at the moment, we only support win32 :P


#if defined(_WIN32) || defined(_WIN64)
#  define ATMA_PLATFORM_WINDOWS true
#  if defined(_WIN64)
#    define ATMA_PLATFORM_WIN64 true
#    define ATMA_POINTER_SIZE 8
#  else
#    define ATMA_PLATFORM_WIN32 true
#    define ATMA_POINTER_SIZE 4
#  endif
//#	pragma warning(push)
//#	pragma warning(disable: 467)
#	include "windows.h"
#	include "windowsx.h"
#	include "xmmintrin.h"
//#	pragma warning(pop)
#	undef min
#	undef max
#endif

#if defined(_MSC_VER)
#	define ATMA_COMPILER_MSVC _MSC_VER
#endif



