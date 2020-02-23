#pragma once

#include <atma/types.hpp>
#include <atma/logging.hpp>


namespace rose
{
	struct console_t
	{
		auto set_color(uint32) -> void;

		auto write(char const*, size_t) -> size_t;

	private:
		console_t();

		uintptr console_handle_;
		uchar console_bg_ = 0x00;
		uchar console_fg_ = 0x07;

		friend struct runtime_t;
	};
}

namespace rose
{
	struct default_console_log_handler_t : atma::logging_handler_t
	{
		default_console_log_handler_t(rose::console_t& console)
			: console_(console)
		{}

		auto handle(atma::log_level_t, atma::unique_memory_t const&) -> void override;

	private:
		rose::console_t& console_;
		atma::log_style_t last_log_style_ = atma::log_style_t::oneline;
	};
}

