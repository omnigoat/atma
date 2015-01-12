#pragma once
//=====================================================================
#include <atma/lockfree/queue.hpp>
#include <vector>
#include <thread>
#include <algorithm>
//=====================================================================
namespace atma {
namespace thread {
//=====================================================================
	
	struct engine_t
	{
		typedef std::function<void()> signal_t;
		typedef atma::lockfree::queue_t<signal_t> queue_t;

		engine_t();
		~engine_t();

		auto signal(signal_t const&) -> void;
		auto signal_evergreen(signal_t const&) -> void;
		auto signal_block() -> void;
		
	private:
		auto reenter(std::atomic_bool const& blocked) -> void;

		std::thread handle_;
		queue_t queue_, evergreen_;
		bool running_;
	};



	inline engine_t::engine_t()
		: running_(true)
	{
		handle_ = std::thread([&]
		{
			// continue performing all commands always and forever
			while (running_)
			{
				for (auto const& x : evergreen_) {
					x();
				}

				signal_t x;
				while (queue_.pop(x)) {
					x();
				}
			}
		});
	}

	inline engine_t::~engine_t()
	{
		signal([&] { running_ = false; });
		handle_.join();
	}


	inline auto engine_t::reenter(std::atomic_bool const& blocked) -> void
	{
		// continue performing all commands always and forever
		signal_t x;
		while (blocked && queue_.pop(x)) {
			x();
		}
	}

	inline auto engine_t::signal(signal_t const& fn) -> void
	{
		if (!running_)
			return;
		queue_.push(fn);
	}

	inline auto engine_t::signal_evergreen(signal_t const& fn) -> void
	{
		if (!running_)
			return;
		evergreen_.push(fn);
	}

	inline auto engine_t::signal_block() -> void
	{
		if (!running_)
			return;

		// push blocking fn!
		auto&& blocked = std::atomic_bool{true};
		queue_.push([&blocked]{ blocked = false; });

		// don't block if we're the prime thread blocking ourselves.
		if (std::this_thread::get_id() == handle_.get_id())
		{
			reenter(blocked);
			return;
		}
		else
		{
			while (blocked)
				;
		}
	}

//=====================================================================
} // namespace thread
} // namespace atma
//=====================================================================
