#pragma once

#include <atma/function.hpp>
#include <atma/lockfree_queue.hpp>
#include <atma/vector.hpp>

#include <atma/config/platform.hpp>
#include <atma/platform/interop.hpp>


// debug, this_thread, etc
namespace atma
{
	namespace this_thread
	{
		inline auto set_debug_name(char const* name)
		{
#if ATMA_PLATFORM_WINDOWS
			auto os_thread_name = platform_interop::make_platform_string(thread_name);
			HRESULT hr = SetThreadDescription(GetCurrentThread(), os_thread_name.get());
#endif
		}
	}

	using thread_id_t = std::thread::id;
}


namespace atma
{
	struct alignas(4) work_token_t
	{
		~work_token_t()
		{
			flags_ |= 1;

			// wait until all our tasks are finished
			while (read_idx != write_idx)
			{}
		}

		auto valid() const { return (flags_ & 1) != 0; }

		auto generate_idx()           -> uint16 { return atma::atomic_post_increment(&write_idx); }
		auto wait_for_idx(uint16 idx) -> void   { while (atma::atomic_load(&read_idx) != idx); }
		auto consume_idx(uint16 idx)  -> void   { ATMA_ENSURE(atma::atomic_compare_exchange(&read_idx, idx, uint16(idx + 1))); }

		template <typename F, typename... Args>
		auto execute_for_idx(uint16 idx, F&& f, Args&&... args)
		{
			using result_type = decltype(std::invoke(std::forward<F>(f), std::forward<Args>(args)...));

			if constexpr (std::is_void_v<result_type>)
			{
				wait_for_idx(idx);
				std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
				consume_idx(idx);
			}
			else
			{
				wait_for_idx(idx);
				auto r = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
				consume_idx(idx);
				return r;
			}
		}

	private:
		uint16 write_idx = 0;
		uint16 read_idx = 0;
		uint32 flags_ = 0;
	};
}

namespace atma
{
	inline void enqueue_function_to_queue(lockfree_queue_t& queue, atma::function<void()> const& f)
	{
		using internal_function_t = basic_relative_function_t<16, void()>;

		queue.with_allocation(sizeof(std::decay_t<decltype(f)>) + (uint32)f.external_buffer_size(), 4, true, [&f](auto& A) {
			new (A.data()) internal_function_t(f, (char*)A.data() + sizeof(internal_function_t));
		});
	}

	template <typename... Args>
	inline void enqueue_function_to_queue(lockfree_queue_t& queue, atma::function<void(Args...)> const& f)
	{
		using internal_function_t = basic_relative_function_t<16, void(Args...)>;

		queue.with_allocation(sizeof(std::decay_t<decltype(f)>) + (uint32)f.external_buffer_size(), 4, true, [&f](auto& A) {
			new (A.data()) internal_function_t(f, (char*)A.data() + sizeof(internal_function_t));
		});
	}

	template <typename... Args>
	inline void consume_queue_of_functions(lockfree_queue_t& queue, Args&&... args)
	{
		using internal_function_t = basic_relative_function_t<16, void(Args...)>;

		while (queue.with_consumption([](auto& D) {
			internal_function_t* f = (internal_function_t*)D.data();
			(*f)(std::forward<Args>(args)...);
			f->~internal_function_t();
		}));
	}
}

// thread_work_provider
namespace atma
{
	struct thread_work_provider_t
	{
	protected:
		using onfly_function_t        = function<void()>;
		using onfly_repeat_function_t = function<bool()>;

	public:
		using function_t        = function<void()>;
		using repeat_function_t = function<bool()>;

		virtual auto is_running() const -> bool = 0;
		virtual auto ensure_running() -> void = 0;
		virtual auto enqueue(function_t const&) -> void = 0;
		virtual auto enqueue(function_t&&) -> void = 0;

		virtual auto enqueue_repeat(repeat_function_t const&) -> void = 0;
		virtual auto enqueue_repeat(repeat_function_t&&) -> void = 0;

		// token-based work
		auto enqueue_against(work_token_t&, function_t const&) -> void;
		auto enqueue_against(work_token_t&, function_t&&) -> void;
		auto enqueue_repeat_against(work_token_t&, repeat_function_t const&) -> void;
		auto enqueue_repeat_against(work_token_t&, repeat_function_t&&) -> void;

		// allow void functions to be repeated forever
		auto enqueue_repeat(function_t const& fn) -> void { enqueue_repeat([fn]() -> bool { fn(); return true; }); }
		auto enqueue_repeat(function_t&& fn) -> void { enqueue_repeat([fn = std::move(fn)]() -> bool { fn(); return true; }); }
		auto enqueue_repeat_against(work_token_t& tk, function_t const& f) -> void { enqueue_repeat_against(tk, [f] { f(); return true; }); }
		auto enqueue_repeat_against(work_token_t& tk, function_t&& f) -> void { enqueue_repeat_against(tk, [f=std::move(f)] { f(); return true; }); }
	};

	inline auto thread_work_provider_t::enqueue_against(work_token_t& tk, function_t const& f) -> void
	{
		auto nf = [&tk, idx=tk.generate_idx(), f]() {
			tk.execute_for_idx(idx, f);
		};

		enqueue(nf);
	}

	inline auto thread_work_provider_t::enqueue_against(work_token_t& tk, function_t&& f) -> void
	{
		auto nf = [&tk, idx=tk.generate_idx(), f=std::move(f)]() {
			tk.execute_for_idx(idx, f);
		};

		enqueue(nf);
	}

	inline auto thread_work_provider_t::enqueue_repeat_against(work_token_t& tk, repeat_function_t const& f) -> void
	{
		// this lambda mutates itself every time it is called to use a different token-index
		auto nf = [&tk, idx=tk.generate_idx(), f]() mutable -> bool
		{
			auto r = tk.execute_for_idx(idx, f);
			auto rr = r && tk.valid();
			if (rr)
				idx = tk.generate_idx();
			return rr;
		};

		enqueue_repeat(nf);
	}

	inline auto thread_work_provider_t::enqueue_repeat_against(work_token_t& tk, repeat_function_t&& f) -> void
	{
		// this lambda mutates itself every time it is called to use a different token-index
		auto nf = [&tk, idx=tk.generate_idx(), f=std::move(f)]() mutable -> bool
		{
			auto r = tk.execute_for_idx(idx, f);
			auto rr = r && tk.valid();
			if (rr)
				idx = tk.generate_idx();
			return rr;
		};

		enqueue_repeat(nf);
	}
}

// inplace_engine_t
namespace atma
{
	struct inplace_engine_t : thread_work_provider_t
	{
		struct defer_start_t {};

		inplace_engine_t();
		inplace_engine_t(defer_start_t, uint32 bufsize);
		explicit inplace_engine_t(uint32 bufsize);
		inplace_engine_t(void* buf, uint32 bufsize);
		~inplace_engine_t();

		auto is_running() const -> bool override;

		auto ensure_running() -> void;
		auto signal(function_t const&) -> void;
		auto signal(function_t&&) -> void;
		auto signal_evergreen(repeat_function_t const&) -> void;
		auto signal_block() -> void;

	protected:
		auto enqueue(function_t const& f) -> void override { signal(f); }
		auto enqueue(function_t&& f) -> void override { signal(std::move(f)); }
		auto enqueue_repeat(repeat_function_t const& fn) -> void override { signal_evergreen(fn); }
		auto enqueue_repeat(repeat_function_t&& fn) -> void override { signal_evergreen(fn); }

	private:
		using queue_t = lockfree_queue_t;
		using queue_fn_t = basic_relative_function_t<16, void()>;

		auto reenter(std::atomic<bool> const& blocked) -> void;

		std::thread handle_;
		queue_t queue_;
		std::atomic<bool> running_;
	};

	inline inplace_engine_t::inplace_engine_t()
		: running_{false}
		, queue_{}
	{}

	inline inplace_engine_t::inplace_engine_t(defer_start_t, uint32 bufsize)
		: running_{false}
		, queue_{bufsize}
	{}

	inline inplace_engine_t::inplace_engine_t(uint32 bufsize)
		: running_{true}
		, queue_{bufsize}
	{
		handle_ = std::thread([&]
		{
			reenter(running_);
		});
	}

	inline inplace_engine_t::inplace_engine_t(void* buf, uint32 bufsize)
		: running_{true}
		, queue_{buf, bufsize}
	{
		handle_ = std::thread([&]
		{
			reenter(running_);
		});
	}

	inline inplace_engine_t::~inplace_engine_t()
	{
		if (running_)
		{
			signal(onfly_function_t{[&] {
				running_ = false;
			}});

			handle_.join();
		}
	}

	inline auto inplace_engine_t::is_running() const -> bool
	{
		return running_;
	}

	inline auto inplace_engine_t::reenter(std::atomic<bool> const& good) -> void
	{
		while (good)
		{
			if (auto D = queue_.consume())
			{
				queue_fn_t* f = (queue_fn_t*)D.data();
				(*f)();
				f->~queue_fn_t();
				queue_.finalize(D);
			}
		}
	}

	inline auto inplace_engine_t::ensure_running() -> void
	{
		if (!is_running())
		{
			running_ = true;
			handle_ = std::thread([&]
			{
				reenter(running_);
			});
		}
	}

	inline auto inplace_engine_t::signal(function_t const& fn) -> void
	{
		if (!running_)
			return;

		auto A = queue_.allocate((uint32)queue_fn_t::contiguous_relative_allocation_size_for(fn), 4, true);
		queue_fn_t::make_contiguous(A.data(), fn);
		queue_.commit(A);
	}

	inline auto inplace_engine_t::signal(function_t&& fn) -> void
	{
		if (!running_)
			return;

		auto A = queue_.allocate((uint32)queue_fn_t::contiguous_relative_allocation_size_for(fn), 4, true);
		queue_fn_t::make_contiguous(A.data(), std::move(fn));
		queue_.commit(A);
	}

	inline auto inplace_engine_t::signal_evergreen(repeat_function_t const& fn) -> void
	{
		auto sg = [&, fn]() {
			if (!fn())
				return;
			signal_evergreen(fn);
		};

		signal(onfly_function_t{sg});
	}

	inline auto inplace_engine_t::signal_block() -> void
	{
		if (!running_)
			return;

		std::atomic<bool> blocked{true};
		signal(onfly_function_t{[&blocked] { blocked = false; }});

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
}


// thread-pool
namespace atma
{
	struct thread_pool_t : thread_work_provider_t
	{
		thread_pool_t(uint threads);
		~thread_pool_t();

		auto thread_count() const -> size_t;
		auto is_running() const -> bool override { return running_; }

		auto ensure_running() -> void override {}

		auto enqueue(function_t const&) -> void override;
		auto enqueue(function_t&&) -> void override;

		auto enqueue_repeat(repeat_function_t const&) -> void override;
		auto enqueue_repeat(repeat_function_t&&) -> void override;

	private:
		using internal_function_t = basic_relative_function_t<16, void()>;

		static auto worker_thread_runloop(thread_pool_t*, std::atomic_bool&) -> void;

	private:
		// queue for work submission
		lockfree_queue_t queue_;
		// threads that pool queue for work
		std::vector<std::thread> threads_;

		std::atomic_bool running_ = true;
	};

	inline thread_pool_t::thread_pool_t(uint threads)
		: queue_{ threads * 256 }
	{
		for (uint i = 0; i != threads; ++i)
			threads_.emplace_back(&worker_thread_runloop, this, std::ref(running_));
	}

	inline thread_pool_t::~thread_pool_t()
	{
		running_ = false;
		for (auto& x : threads_)
			x.join();
	}

	inline auto thread_pool_t::thread_count() const -> size_t
	{
		return threads_.size();
	}

	inline auto thread_pool_t::enqueue(function_t const& fn) -> void
	{
		queue_.with_allocation(sizeof(internal_function_t) + (uint32)fn.external_buffer_size(), 4, true, [&fn](auto& A) {
			new (A.data()) internal_function_t{fn, (char*)A.data() + sizeof(internal_function_t)};
		});
	}

	inline auto thread_pool_t::enqueue(function_t&& fn) -> void
	{
		queue_.with_allocation(sizeof(internal_function_t) + (uint32)fn.external_buffer_size(), 4, true, [&fn](auto& A) {
			new (A.data()) internal_function_t{std::move(fn), (char*)A.data() + sizeof(internal_function_t)};
		});
	}

	inline auto thread_pool_t::enqueue_repeat(repeat_function_t const& fn) -> void
	{
		onfly_function_t rfn = [&, fn]{
			if (!fn())
				return;
			enqueue_repeat(fn);
		};

		queue_.with_allocation(sizeof(internal_function_t) + (uint32)rfn.external_buffer_size(), 4, true, [&rfn](auto& A) {
			new (A.data()) internal_function_t{rfn, (char*)A.data() + sizeof(internal_function_t)};
		});
	}

	inline auto thread_pool_t::enqueue_repeat(repeat_function_t&& fn) -> void
	{
		onfly_function_t rfn = [&, fn = std::move(fn)]{
			if (!fn())
				return;
			enqueue_repeat(std::move(fn));
		};

		queue_.with_allocation(sizeof(internal_function_t) + (uint32)rfn.external_buffer_size(), 4, true, [&rfn](auto& A) {
			new (A.data()) internal_function_t{rfn, (char*)A.data() + sizeof(internal_function_t)};
		});
	}

	inline auto thread_pool_t::worker_thread_runloop(thread_pool_t* pool, std::atomic_bool& running) -> void
	{
		char buf[128];
		int r = sprintf(buf, "threadpool %#0jx worker", (uintmax_t)pool);
		atma::this_thread::set_debug_name(buf);

		//using lfn_t = atma::relative_function<

		atma::unique_memory_t mem;
		while (running)
		{
			if (pool->queue_.with_consumption([&](auto& D)
			{
				D.local_copy(mem);
			}))
			{
				internal_function_t* f = (internal_function_t*)mem.begin();
				(*f)();
			}

			Sleep(1);
		}
	}
}
