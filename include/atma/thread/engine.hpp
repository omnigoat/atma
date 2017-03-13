#pragma once

#include <atma/lockfree/queue.hpp>
#include <atma/mpsc_queue.hpp>
#include <atma/function.hpp>

#include <vector>
#include <thread>


namespace atma { namespace thread {

	struct engine_t
	{
		using signal_t = std::function<void()>;
		using queue_t  = atma::lockfree::queue_t<signal_t>;
		using batch_t  = queue_t::batch_t;

		struct defer_start_t {};

		engine_t();
		engine_t(defer_start_t);
		~engine_t();

		auto is_running() const -> bool { return running_.load(); }
		auto start() -> void;

		auto signal(signal_t const&) -> void;
		auto signal_batch(batch_t&) -> void;
		auto signal_evergreen(signal_t const&) -> void;
		auto signal_block() -> void;

	private:
		auto reenter(std::atomic<bool> const& blocked) -> void;

		std::thread handle_;
		queue_t queue_;
		std::atomic<bool> running_;
	};




	inline engine_t::engine_t()
		: running_{true}
	{
		handle_ = std::thread([&]
		{
			reenter(running_);
		});
	}

	inline engine_t::engine_t(defer_start_t)
		: running_{false}
	{
	}

	inline engine_t::~engine_t()
	{
		signal([&] {
			running_ = false;
		});

		handle_.join();
	}

	inline auto engine_t::start() -> void
	{
		running_ = true;

		auto t = std::thread([&] { reenter(running_); });

		handle_.swap(t);
	}

	inline auto engine_t::reenter(std::atomic<bool> const& good) -> void
	{
		signal_t x;
		while (good) {
			if (queue_.pop(x))
				x();
		}
	}

	inline auto engine_t::signal(signal_t const& fn) -> void
	{
		if (!running_)
			return;

		queue_.push(fn);
	}

	inline auto engine_t::signal_batch(batch_t& batch) -> void
	{
		if (!running_)
			return;

		queue_.push(batch);
	}

	inline auto engine_t::signal_evergreen(signal_t const& fn) -> void
	{
		auto sg = [&, fn]() {
			fn();
			signal_evergreen(fn);
		};

		signal(sg);
	}

	inline auto engine_t::signal_block() -> void
	{
		if (!running_)
			return;

		std::atomic<bool> blocked{true};
		queue_.push([&blocked]{
			blocked = false;
		});

		// the engine thread can't block itself!
		if (std::this_thread::get_id() == handle_.get_id())
		{
			reenter(blocked);
		}
		else
		{
			while (blocked)
				;
		}
	}

} }


