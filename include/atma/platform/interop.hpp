#pragma once

#include <atma/config/platform.hpp>

// string
namespace atma::platform_interop
{
#if defined(ATMA_PLATFORM_WINDOWS)
	using string_t = wchar_t const*;
#endif
}


namespace atma::platform_interop
{
#if defined(ATMA_PLATFORM_WINDOWS)
	inline auto make_platform_string(char const* str) -> std::unique_ptr<wchar_t[]>
	{
		int char16s = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);
		wchar_t* wstr = new wchar_t[char16s];
		int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, wstr, char16s);

		if (r == 0) {
			delete wstr;
			return std::unique_ptr<wchar_t[]>{};
		}
		else {
			return std::unique_ptr<wchar_t[]>{wstr};
		}
	}
#endif
}
