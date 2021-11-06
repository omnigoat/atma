#pragma once

#include <rose/rose_fwd.hpp>
#include <rose/path.hpp>
#include <rose/console.hpp>

#include <atma/string.hpp>
#include <atma/function.hpp>
#include <atma/threading.hpp>

#include <memory>
#include <map>
#include <chrono>
#include <set>

import atma.vector;


namespace rose
{
	struct runtime_t
	{
		using file_change_callback_t = atma::function<void(path_t const&, file_change_t)>;
		using dir_watchers_t = std::map<path_t, size_t>;
		using dir_watch_handle_t = intptr_t;

		runtime_t();
		runtime_t(atma::thread_work_provider_t*);
		~runtime_t();

		// console
		auto console() -> console_t& { return console_; }
		auto console_logging_handler() -> atma::logging_handler_t* { return &default_console_log_handler_; }

		// file-watch
		auto register_directory_watch(
			path_t const&,
			bool recursive,
			file_change_mask_t,
			file_change_callback_t const&) -> void;

	private: // directory watching
		struct dir_watch_t;

		using dir_watch_handles_t = atma::vector<HANDLE>;
		using dir_watch_infos_t = atma::vector<dir_watch_t>;

		auto initialize_watching() -> void;

	private:
		console_t console_;

		// we must provide an easy way to log stuff
		default_console_log_handler_t default_console_log_handler_;

		// directory watching
		dir_watch_handles_t dir_handles_;
		dir_watch_infos_t dir_infos_;
		dir_watchers_t dir_watchers_;

		// placed last for good reason (other thread still using members!)
		atma::inplace_engine_t filewatch_engine_;
		atma::thread_work_provider_t* work_provider_;
		atma::work_token_t token_;
		std::atomic_bool running_ = false;

		friend VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
	};

	VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);


	struct runtime_t::dir_watch_t
	{
		using callbacks_t = std::vector<file_change_callback_t>;

		static const int bufsize = 512;

		OVERLAPPED overlapped;
		path_t path;
		alignas(4) char bufs[2][bufsize];
		uint32 bufidx = 0;
		uint32 notify;
		HANDLE handle;
		callbacks_t callbacks;

		// changes
		bool pending_change = false;
		std::chrono::time_point<std::chrono::high_resolution_clock> trigger;
		std::set<std::tuple<atma::string, file_change_t>> files;
	};

	inline auto setup_default_logging_to_console(runtime_t& RR) -> void
	{
		atma::default_logging_runtime()->attach_handler(RR.console_logging_handler());
	}


}
