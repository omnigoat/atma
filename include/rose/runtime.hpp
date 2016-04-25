#pragma once

#include <rose/console.hpp>


namespace rose
{
	struct runtime_t
	{
		auto initialize_console() -> void;
		
		auto get_console() const -> console_t& { return console_; }

	private:
		console_t* console_;
	};
}
