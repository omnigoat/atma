#pragma once

#include <rose/rose_fwd.hpp>
#include <rose/path.hpp>

#include <atma/string.hpp>
#include <atma/function.hpp>
#include <atma/thread/engine.hpp>
#include <atma/vector.hpp>

#include <memory>
#include <map>


namespace rose
{
	struct console_t;

	struct runtime_t
	{
		using file_change_callback_t = atma::function<void(path_t const&, file_change_t)>;
		using dir_watchers_t = std::map<path_t, size_t>;
		using dir_watch_handle_t = intptr_t;

		runtime_t();
		~runtime_t();

		// console
		auto initialize_console() -> void;
		auto get_console() -> console_t&;

		// file-watch
		auto register_directory_watch(
			path_t const&,
			bool recursive,
			file_change_mask_t,
			file_change_callback_t const&) -> void;

	private:
		std::unique_ptr<console_t> console_;

	private: // directory watching
		struct dir_watch_t;

		using dir_watch_handles_t = atma::vector<HANDLE>;
		using dir_watch_infos_t = atma::vector<dir_watch_t>;

		auto initialize_watching() -> void;

		atma::thread::engine_t filewatch_engine_;

		dir_watch_handles_t dir_handles_;
		dir_watch_infos_t dir_infos_;
		dir_watchers_t dir_watchers_;

		friend VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
	};


	struct runtime_t::dir_watch_t
	{
		using callbacks_t = std::vector<file_change_callback_t>;

		static const int bufsize = 512;

		OVERLAPPED overlapped;
		path_t path;
		alignas(4) char buf[bufsize];
		uint32 notify;
		HANDLE handle;
		callbacks_t callbacks;
	};
}
