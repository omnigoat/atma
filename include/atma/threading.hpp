#pragma once

#include <atma/config/platform.hpp>
#include <atma/function.hpp>
#include <atma/mpsc_queue.hpp>
#include <atma/vector.hpp>


// debug, this_thread, etc
namespace atma
{
#if ATMA_PLATFORM_WINDOWS
	namespace detail
	{
		const DWORD MS_VC_EXCEPTION = 0x406D1388;
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType; // Must be 0x1000.
			LPCSTR szName; // Pointer to name (in user addr space).
			DWORD dwThreadID; // Thread ID (-1=caller thread).
			DWORD dwFlags; // Reserved for future use, must be zero.
		} THREADNAME_INFO;

		inline void set_debug_name_impl(DWORD dwThreadID, const char* threadName)
		{
			THREADNAME_INFO info = {0x1000, threadName, dwThreadID, 0};

			__try
			{
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	}
#endif

	namespace this_thread
	{
		inline auto set_debug_name(char const* name)
		{
			detail::set_debug_name_impl(-1, name);
		}
	}
}

//
namespace atma
{
	struct thread_work_provider_t
	{
		using function_t = basic_generic_function_t<sizeof(void*), atma::functor_storage_t::heap, void()>;
		using repeat_function_t = basic_generic_function_t<sizeof(void*), atma::functor_storage_t::heap, bool()>;

		virtual auto is_running() const -> bool = 0;
		virtual auto ensure_running() -> void = 0;
		virtual auto enqueue(function_t const&) -> void = 0;
		virtual auto enqueue(function_t&&) -> void = 0;

		virtual auto enqueue_repeat(repeat_function_t const&) -> void = 0;
		virtual auto enqueue_repeat(repeat_function_t&&) -> void = 0;

		auto enqueue_repeat(function_t const& fn) -> void
		{
			enqueue_repeat([fn]() -> bool { fn(); return true; });
		}

		auto enqueue_repeat(function_t&& fn) -> void
		{
			enqueue_repeat([fn = std::move(fn)]() -> bool { fn(); return true; });
		}
	};
}

// inplace_engine_t
namespace atma
{
	struct inplace_engine_t : thread_work_provider_t
	{
		struct defer_start_t {};

		explicit inplace_engine_t(defer_start_t, uint32 bufsize);
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
		using queue_t = mpsc_queue_t<false>;
		
		auto reenter(std::atomic<bool> const& blocked) -> void;

		std::thread handle_;
		queue_t queue_;
		std::atomic<bool> running_;
	};


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
			signal([&] {
				running_ = false;
			});

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
			atma::vector<char> buf;
			function_t* f = nullptr;
			if (queue_.with_consumption([&](auto& D)
			{
				buf.resize(D.size());
				f = (function_t*)buf.data();
				D.decode_struct(*f);
				f->relocate_external_buffer(buf.data() + sizeof(function_t));
			}))
			{
				(*f)();
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

		auto A = queue_.allocate(sizeof(function_t) + (uint32)fn.external_buffer_size(), 4, true);
		new (A.data()) function_t{fn, (char*)A.data() + sizeof(function_t)};
		queue_.commit(A);
	}

	inline auto inplace_engine_t::signal(function_t&& fn) -> void
	{
		if (!running_)
			return;

		auto A = queue_.allocate(sizeof(function_t) + (uint32)fn.external_buffer_size(), 4, true);
		new (A.data()) function_t{std::move(fn), (char*)A.data() + sizeof(function_t)};
		queue_.commit(A);
	}

	inline auto inplace_engine_t::signal_evergreen(repeat_function_t const& fn) -> void
	{
		auto sg = [&, fn]() {
			if (!fn())
				return;
			signal_evergreen(fn);
		};

		signal(sg);
	}

	inline auto inplace_engine_t::signal_block() -> void
	{
		if (!running_)
			return;

		std::atomic<bool> blocked{true};
		signal([&blocked] { blocked = false; });

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
		using function_t = basic_function_t<sizeof(void*), void()>;

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
		static auto worker_thread_runloop(thread_pool_t*, std::atomic_bool&) -> void;

	private:
		// queue for work submission
		mpsc_queue_t<false> queue_;
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
		queue_.with_allocation(sizeof(function_t) + (uint32)fn.external_buffer_size(), 4, true, [&fn](auto& A) {
			new (A.data()) function_t{fn, (char*)A.data() + sizeof(function_t)};
		});
	}

	inline auto thread_pool_t::enqueue(function_t&& fn) -> void
	{
		queue_.with_allocation(sizeof(function_t) + (uint32)fn.external_buffer_size(), 4, true, [&fn](auto& A) {
			new (A.data()) function_t{std::move(fn), (char*)A.data() + sizeof(function_t)};
		});
	}

	inline auto thread_pool_t::enqueue_repeat(repeat_function_t const& fn) -> void
	{
		function_t rfn = [&, fn]{
			if (!fn())
				return;
			enqueue_repeat(fn);
		};

		queue_.with_allocation(sizeof(function_t) + (uint32)rfn.external_buffer_size(), 4, true, [&rfn](auto& A) {
			new (A.data()) function_t{rfn, (char*)A.data() + sizeof(function_t)};
		});
	}

	inline auto thread_pool_t::enqueue_repeat(repeat_function_t&& fn) -> void
	{
		function_t rfn = [&, fn = std::move(fn)]{
			if (!fn())
				return;
			enqueue_repeat(std::move(fn));
		};

		queue_.with_allocation(sizeof(function_t) + (uint32)rfn.external_buffer_size(), 4, true, [&rfn](auto& A) {
			new (A.data()) function_t{rfn, (char*)A.data() + sizeof(function_t)};
		});
	}

	inline auto thread_pool_t::worker_thread_runloop(thread_pool_t* pool, std::atomic_bool& running) -> void
	{
		char buf[128];
		int r = sprintf(buf, "threadpool %#0jx worker", (uintmax_t)pool);
		atma::this_thread::set_debug_name(buf);

		atma::unique_memory_t mem;
		while (running)
		{
			if (pool->queue_.with_consumption([&](auto& D)
			{
				D.local_copy(mem);
				function_t* f = (function_t*)mem.begin();
				f->relocate_external_buffer(mem.begin() + sizeof(function_t));
			}))
			{
				function_t* f = (function_t*)mem.begin();
				(*f)();
			}

			Sleep(1);
		}
	}
}
