#pragma once

#include <atma/config/platform.hpp>


namespace atma
{
#if ATMA_PLATFORM_WINDOWS
	namespace detail
	{
		const DWORD MS_VC_EXCEPTION = 0x406D1388;
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType; // Must be 0x1000.
			LPCSTR szName; // Pointer to name (in user addr space).
			DWORD dwThreadID; // Thread ID (-1=caller thread).
			DWORD dwFlags; // Reserved for future use, must be zero.
		} THREADNAME_INFO;

		inline void set_debug_name_impl(DWORD dwThreadID, const char* threadName)
		{
			THREADNAME_INFO info = {0x1000, threadName, dwThreadID, 0};

			__try
			{
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	}
#endif

	namespace this_thread
	{
		inline auto set_debug_name(char const* name)
		{
			detail::set_debug_name_impl(-1, name);
		}
	}
}

