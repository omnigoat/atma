#pragma once

#include <atma/types.hpp>


namespace rose
{
	struct console_t
	{
		auto write(char const*, size_t) -> size_t;

	private:
		console_t();

		uintptr console_handle_;

		friend struct runtime_t;
	};
}
