#pragma once

#include <atma/types.hpp>


namespace rose
{
	struct console_t
	{
		auto set_color(uint32) -> void;

		auto write(char const*, size_t) -> size_t;

	private:
		console_t();

		uintptr console_handle_;
		byte console_bg_ = 0x00;
		byte console_fg_ = 0x07;

		friend struct runtime_t;
	};
}
