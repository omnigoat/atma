#pragma once

#include <rose/rose_fwd.hpp>

#include <atma/string.hpp>
#include <atma/function.hpp>

#include <memory>


namespace rose
{
	struct console_t;

	struct runtime_t
	{
		~runtime_t();

		// console
		auto initialize_console() -> void;
		auto get_console() -> console_t&;

		// file-watch
		auto register_directory_watch(
			atma::string const& path,
			bool recursive,
			file_change_mask_t,
			atma::function<void(atma::string const&, file_change_t)>) -> void;

	private:
		std::unique_ptr<console_t> console_;
	};
}
