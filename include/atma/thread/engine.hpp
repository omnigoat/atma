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

		engine_t();
		~engine_t();

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

	inline engine_t::~engine_t()
	{
		signal([&] {
			running_ = false;
		});

		handle_.join();
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





namespace atma { namespace thread {

	template <bool Dynamic>
	struct inplace_engine_t
	{
		using signal_t = basic_function_t<sizeof(void*), void()>;
		using queue_t  = mpsc_queue_t<Dynamic>;

		inplace_engine_t(uint32 bufsize);
		inplace_engine_t(void* buf, uint32 bufsize);
		~inplace_engine_t();

		template <typename F> auto signal(F&&) -> void;
		template <typename F> auto signal_evergreen(F&&) -> void;
		auto signal_block() -> void;

	private:
		auto reenter(std::atomic<bool> const& blocked) -> void;

		std::thread handle_;
		queue_t queue_;
		std::atomic<bool> running_;
	};


	template <bool D>
	inline inplace_engine_t<D>::inplace_engine_t(uint32 bufsize)
		: running_{true}
		, queue_{bufsize}
	{
		handle_ = std::thread([&]
		{
			reenter(running_);
		});
	}

	template <bool D>
	inline inplace_engine_t<D>::inplace_engine_t(void* buf, uint32 bufsize)
		: running_{true}
		, queue_{buf, bufsize}
	{
		handle_ = std::thread([&]
		{
			reenter(running_);
		});
	}

	template <bool D>
	inline inplace_engine_t<D>::~inplace_engine_t()
	{
		signal([&] {
			running_ = false;
		});

		handle_.join();
	}

	template <bool D>
	inline auto inplace_engine_t<D>::reenter(std::atomic<bool> const& good) -> void
	{
		while (good) {
			if (auto D = queue_.consume()) {
				signal_t* f;
				D.decode_pointer(f);
				(*f)();
				queue_.finalize(D);
			}
		}
	}

	template <bool D>
	template <typename F>
	inline auto inplace_engine_t<D>::signal(F&& f) -> void
	{
		if (!running_)
			return;

		auto A = queue_.allocate(sizeof(signal_t) + sizeof(std::decay_t<F>), 1, true);
		new (A.data()) signal_t{(char*)A.data() + sizeof(signal_t), std::forward<F>(f)};
		queue_.commit(A);
	}

	template <bool D>
	template <typename F>
	inline auto inplace_engine_t<D>::signal_evergreen(F&& fn) -> void
	{
		auto sg = [&, fn]() {
			fn();
			signal_evergreen(fn);
		};

		signal(sg);
	}

	template <bool D>
	inline auto inplace_engine_t<D>::signal_block() -> void
	{
		if (!running_)
			return;

		std::atomic<bool> blocked{true};
		signal([&blocked]{ blocked = false; });

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

