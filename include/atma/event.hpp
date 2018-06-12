#pragma once

#include <atma/lockfree_queue.hpp>
#include <atma/flyweight.hpp>
#include <atma/function.hpp>
#include <atma/threading.hpp>
#include <atma/algorithm.hpp>
#include <atma/arena_allocator.hpp>
#include <atma/ranges/filter.hpp>

#include <functional>
#include <memory>
#include <unordered_map>
#include <map>
#include <tuple>


namespace atma
{
	template <typename... Args>
	struct event_t;

	struct event_binder_t;

	struct event_system_t;
}

namespace atma::detail
{
	struct event_backend_t;
	using event_backend_ptr = intrusive_ptr<event_backend_t>;

	struct event_system_backend_t;
	using event_system_backend_ptr = intrusive_ptr<event_system_backend_t>;
}


// base-binding-info
namespace atma::detail
{
	struct base_binding_info_t
	{
		base_binding_info_t(event_backend_ptr const& eb, thread_id_t id)
			: event_backend{eb}
			, thread_id{id}
		{}

		virtual ~base_binding_info_t() = default;

		event_backend_ptr event_backend;
		thread_id_t thread_id;
	};

	using base_binding_info_ptr = std::unique_ptr<base_binding_info_t>;
	using base_binding_infos_t = std::list<base_binding_info_ptr>;

	template <typename... Args>
	struct binding_info_t : base_binding_info_t
	{
		template <typename F>
		binding_info_t(event_backend_ptr const& eb, F&& f, thread_id_t id)
			: base_binding_info_t{eb, id}
			, f{std::forward<F>(f)}
		{}

		~binding_info_t() override = default;

		atma::function<void(Args...)> f;
	};
	
	template <typename... Args>
	using binding_info_ptr = std::unique_ptr<binding_info_t<Args...>>;

}


// event-system-backend
namespace atma::detail
{
	struct event_system_backend_t : atma::ref_counted
	{
		using queues_t = std::map<thread_id_t, lockfree_queue_t>;

		event_system_backend_t();
		event_system_backend_t(event_system_backend_t const&) = delete;

		template <typename... Args, typename F>
		auto bind(event_backend_ptr const& event_backend, event_binder_t& binder, thread_id_t thread_id, F&& f) -> void;

		auto unbind(event_backend_ptr const&, base_binding_infos_t::iterator const&) -> void;

		template <typename... Args>
		auto raise(event_backend_t const*, thread_id_t thread_id, Args... args) -> void;

		auto process_events_for(thread_id_t thread_id) -> void;

	private:
		auto per_thread_queue(thread_id_t id) -> lockfree_queue_t&;

	private:
		queues_t queues_;

		base_binding_infos_t bindings_;
		std::mutex bindings_mutex_;

		// memory-resource for the per-thread argument copies
		atma::arena_memory_resource_t threaded_args_resource;

		// these are the event-backends that have bindings through us
		std::vector<std::tuple<event_backend_ptr, int>> event_backends_;

		friend struct ::atma::detail::event_backend_t;
	};
}


// event-backend
namespace atma::detail
{
	struct event_backend_t : ref_counted_of<event_backend_t>
	{
		vector<event_system_backend_ptr> event_systems;
		std::mutex bound_systems_mutex;

		event_backend_t() = default;
		event_backend_t(event_backend_t const&) = delete;

		template <typename... Args, typename F>
		void bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f);

		template <typename... Args>
		void raise(Args...);
	};

}

// event-system
namespace atma
{
	struct event_system_t : flyweight_t<detail::event_system_backend_t>
	{
		using flyweight_t<detail::event_system_backend_t>::backend;
		using flyweight_t<detail::event_system_backend_t>::backend_ptr;

		// bind
		template <typename F, typename... Args>
		auto bind(event_t<Args...>& e, event_binder_t& b, F&& f) -> void;

		template <typename F, typename... Args>
		auto bind(event_t<Args...>& e, event_binder_t& b, thread_id_t thread_id, F&& f) -> void;

		// raise
		template <typename... EArgs>
		auto raise(event_t<EArgs...>& e, EArgs... args) -> void;

		auto process_events_for_this_thread() -> void;
	};
}


// SERIOUSLY, event goes here


// event-binder
namespace atma
{
	struct event_binder_t
	{
		event_binder_t() = default;
		event_binder_t(event_binder_t const&) = delete;

		~event_binder_t()
		{
			if (event_backend_)
			{
				ATMA_SCOPED_LOCK(event_backend_->bound_systems_mutex)
				{
					event_system_backend_->unbind(event_backend_, binding_handle_);
				}
			}
		}

		auto is_bound() const -> bool { return !!event_backend_; }

	private:
		detail::event_backend_ptr event_backend_;
		detail::event_system_backend_ptr event_system_backend_;
		detail::base_binding_infos_t::iterator binding_handle_;

		friend struct ::atma::detail::event_system_backend_t;
	};
}



















// event-system-backend implementation
namespace atma::detail
{
	template <typename Range>
	inline auto find_event_backend(Range&& range, event_backend_ptr const& eb)
	{
		return std::find_if(std::begin(range), std::end(range),
			[&eb](auto&& x) { return std::get<0>(x) == eb; });
	}

	inline event_system_backend_t::event_system_backend_t()
		: threaded_args_resource{16, 64, 32}
	{}

	template <typename... Args, typename F>
	inline auto event_system_backend_t::bind(event_backend_ptr const& event_backend, event_binder_t& binder, thread_id_t thread_id, F&& f) -> void
	{
		ATMA_SCOPED_LOCK(bindings_mutex_)
		{
			// register the event-backend with us
			if (auto candidate = find_event_backend(event_backends_, event_backend); candidate == event_backends_.end())
				event_backends_.emplace_back(event_backend, 1);
			else
				++std::get<1>(*candidate);

			// store the new binding
			auto binding_handle = bindings_.insert(bindings_.end(), std::make_unique<binding_info_t<Args...>>(event_backend, std::forward<F>(f), thread_id));

			// put iter in binder
			binder.event_backend_ = event_backend;
			binder.event_system_backend_ = shared_from_this<event_system_backend_t>();
			binder.binding_handle_ = binding_handle;
		}
	}

	template <typename... Args>
	inline auto event_system_backend_t::raise(event_backend_t const* eb, thread_id_t thread_id, Args... args) -> void
	{
		using tuple_type = std::tuple<Args...>;

		auto this_thread_id = std::this_thread::get_id();

		auto is_event = [&](auto&& x) { return x->event_backend == eb; };
		auto is_remote_event = [&](auto&& x) { return is_event(x) && x->thread_id != thread_id_t{} && x->thread_id != this_thread_id; };
		auto is_immediate_event = [&](auto&& x) { return is_event(x) && (x->thread_id == thread_id_t{} || x->thread_id == this_thread_id); };
		auto cast = [](auto&& x) -> binding_info_t<Args...>& { return *static_cast<binding_info_t<Args...>*>(x.get()); };

		// no thread specified, free to schedule as normal
		if (thread_id == thread_id_t{})
		{
			std::map<thread_id_t, tuple_type*> argument_store;

			// first, queue all tethered (to other thread) bindings
			for (auto&& binding : map(cast, filter(is_remote_event, bindings_)))
			{
				// create a copy of the arguments for the requested thread (if we haven't already)
				auto candidate = argument_store.find(binding.thread_id);
				if (candidate == argument_store.end())
				{
					auto tuple_args_ptr = (tuple_type*)threaded_args_resource.allocate(sizeof(tuple_type));
					new (tuple_args_ptr) tuple_type{args...};
					auto[riter, r] = argument_store.insert({binding.thread_id, tuple_args_ptr});
					ATMA_ASSERT(r);
					candidate = riter;
				}

				// using the per-thread copy of the arguments, enqueue the command
				auto& tupled_arguments = *candidate->second;
				atma::enqueue_function_to_queue(
					per_thread_queue(binding.thread_id),
					std::function<void()>{[f = binding.f, &tupled_arguments]{std::apply(f, tupled_arguments); }});
			}

			// enqueue the deletion of the per-thread argument copies
			for (auto const&[arg_thread_id, tuple_args_ptr] : argument_store)
			{
				atma::enqueue_function_to_queue(
					per_thread_queue(arg_thread_id),
					[&, tuple_args_ptr] { threaded_args_resource.deallocate(tuple_args_ptr, sizeof(tuple_type)); });
			}

			// execute the rest immediately
			for (auto&& binding : map(cast, filter(is_immediate_event, bindings_)))
			{
				std::invoke(binding.f, args...);
			}
		}
		// thread specified, we need to exclude tethered bindings and "move" thread-wandering bindings
		else
		{
			for (auto&& binding : map(cast, filter(is_event, bindings_)))
			{
				// thread-tethered binding for this very thread, run immediately
				if (binding.thread_id == thread_id && thread_id == this_thread_id)
				{
					std::invoke(binding.f, args...);
				}
				// thread-tethered binding for another thread, or thread-wandering binding, enqueue for the requested thread
				else if (binding.thread_id == thread_id || binding.thread_id == thread_id_t{})
				{
					atma::enqueue_function_to_queue(
						per_thread_queue(thread_id),
						std::function<void()>{atma::bind(binding.f, args...)});
				}
			}
		}

	}

	inline auto event_system_backend_t::unbind(event_backend_ptr const& eb, base_binding_infos_t::iterator const& x) -> void
	{
		ATMA_SCOPED_LOCK(bindings_mutex_)
		{
			bindings_.erase(x);

			if (auto candidate = find_event_backend(event_backends_, eb); candidate != event_backends_.end())
				if (--std::get<1>(*candidate) == 0)
					event_backends_.erase(candidate);
		}
	}

	inline auto event_system_backend_t::process_events_for(thread_id_t thread_id) -> void
	{
		auto candidate = queues_.find(thread_id);
		if (candidate == queues_.end())
			return;

		atma::consume_queue_of_functions<>(candidate->second);
	}

	inline auto event_system_backend_t::per_thread_queue(thread_id_t id) -> lockfree_queue_t&
	{
		auto candidate = queues_.find(id);
		if (candidate == queues_.end())
		{
			auto[iter, success] = queues_.emplace(id, 2048);
			ATMA_ASSERT(success);
			candidate = iter;
		}

		return candidate->second;
	}
}


// event-backend implementation
namespace atma::detail
{
	template <typename... Args, typename F>
	inline void event_backend_t::bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f)
	{
		auto const& esbp = event_system.backend_ptr();

		ATMA_SCOPED_LOCK(bound_systems_mutex)
		{
			auto candidate = atma::find_in(event_systems, esbp);
			if (candidate == event_systems.end())
				candidate = event_systems.insert(event_systems.end(), esbp);

			esbp->bind<Args...>(shared_from_this(), binder, thread_id, std::forward<F>(f));
		}
	}

	template <typename... Args>
	inline void event_backend_t::raise(Args... args)
	{
		ATMA_SCOPED_LOCK(bound_systems_mutex)
		{
			for (auto&& system : event_systems)
			{
				if (!system)
					continue;

				system->raise<Args...>(this, thread_id_t{}, std::forward<Args>(args)...);
			}
		}
	}
}


// event-system implementation
namespace atma
{
	template <typename F, typename... Args>
	inline auto event_system_t::bind(event_t<Args...>& e, event_binder_t& b, F&& f) -> void
	{
		backend().bind(e, b, std::forward<F>(f), std::this_thread::get_id());
	}

	template <typename F, typename... Args>
	inline auto event_system_t::bind(event_t<Args...>& e, event_binder_t& b, thread_id_t thread_id, F&& f) -> void
	{
		backend().bind(e.backend_, b, thread_id, std::forward<F>(f));
	}

	template <typename... EArgs>
	inline auto event_system_t::raise(event_t<EArgs...>& e, EArgs... args) -> void
	{
		backend().raise(e, args...);
	}

	inline auto event_system_t::process_events_for_this_thread() -> void
	{
		backend().process_events_for(std::this_thread::get_id());
	}
}


// event
namespace atma
{
	template <typename F>
	struct bound_event_t
	{
		event_binder_t& b;
		F f;
	};


	template <typename... Args>
	struct event_t  : flyweight_t<detail::event_backend_t>
	{
		event_t() = default;
		event_t(event_t const&) = default;

		template <typename... RArgs>
		auto raise(RArgs&&... args) -> void
		{
			backend().raise<Args...>(std::forward<RArgs>(args)...);
		}

		// bind, fully-specified
		template <typename F>
		void bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f)
		{
			backend().bind<Args...>(event_system, binder, thread_id, std::forward<F>(f));
		}

		// bind, untethered
		template <typename F>
		void bind(event_system_t& event_system, event_binder_t& binder, F&& f)
		{
			backend().bind<Args...>(event_system, binder, thread_id_t{}, std::forward<F>(f));
		}

		// bind, tethered but default
		template <typename F>
		void bind(F&& f, thread_id_t id)
		{
			detail::default_event_system.bind(*this, detail::default_event_binder, std::forward<F>(f), id);
		}
		
		// bind, fully-unspecified
		template <typename F>
		void bind(F&& f)
		{
			this->bind(detail::default_event_system, detail::default_event_binder, std::this_thread::get_id(), std::forward<F>(f));
		}

		template <typename F>
		event_t& operator += (F&& f)
		{
			detail::default_event_system.bind(*this, detail::default_event_binder, std::forward<F>(f));
			return *this;
		}

		template <typename F>
		event_t& operator += (bound_event_t<F>&& be)
		{
			detail::default_event_system.bind(*this, be.b, std::forward<F>(be.f));
			return *this;
		}

	private:
		friend struct ::atma::event_system_t;
		friend struct ::atma::detail::event_system_backend_t;
	};

	


}

namespace atma::detail
{
	inline event_system_t default_event_system;
	inline event_binder_t default_event_binder;
}
