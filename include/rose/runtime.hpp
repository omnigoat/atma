#pragma once

#include <memory>


namespace rose
{
	struct console_t;

	struct runtime_t
	{
		~runtime_t();

		auto initialize_console() -> void;
		
		auto get_console() -> console_t&;

	private:
		std::unique_ptr<console_t> console_;
	};
}
