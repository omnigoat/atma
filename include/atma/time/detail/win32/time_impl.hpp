//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_TIME_DETAIL_WIN32_TIME_IMPL_HPP
#define ATMA_TIME_DETAIL_WIN32_TIME_IMPL_HPP
//=====================================================================
#include <windows.h>
//=====================================================================
namespace atma {
//=====================================================================
	
	inline time_t time()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return li.QuadPart;
	}
	
	inline double convert_time_to_seconds(const time_t& t)
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		return t / double(li.QuadPart);
	}
	
//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================