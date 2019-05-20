#pragma once

#if defined(_WIN32) || defined(_WIN64)
#  define ATMA_PLATFORM_WINDOWS true
#  if defined(_WIN64)
#    define ATMA_PLATFORM_WIN64 true
#    define ATMA_POINTER_SIZE 8
#  else
#    define ATMA_PLATFORM_WIN32 true
#    define ATMA_POINTER_SIZE 4
#  endif
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  include <xmmintrin.h>
#  undef min
#  undef max
#endif

#if defined(_MSC_VER)
#	define ATMA_COMPILER_MSVC _MSC_VER
#endif

namespace atma
{
	template <size_t Bytes = 0>
	struct alignas(64) cache_line_pad_t
	{
	private:
		char pad[64 - Bytes];
	};
}

