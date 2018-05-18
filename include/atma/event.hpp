#pragma once

#include <atma/lockfree_queue.hpp>
#include <atma/flyweight.hpp>
#include <atma/function.hpp>
#include <atma/threading.hpp>
#include <atma/algorithm.hpp>
#include <atma/arena_allocator.hpp>
#include <atma/algorithm/filter.hpp>

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

	struct base_binding_info_t
	{
		virtual ~base_binding_info_t() {}
	};

	using base_binding_info_ptr = std::unique_ptr<base_binding_info_t>;
	using base_binding_infos_t = std::list<base_binding_info_ptr>;

	template <typename... Args>
	struct binding_info_t : base_binding_info_t
	{
		template <typename F>
		binding_info_t(F&& f, thread_id_t id)
			: f{std::forward<F>(f)}
			, thread_id{id}
		{}

		~binding_info_t() override = default;

		atma::function<void(Args...)> f;
		thread_id_t thread_id;
	};
	
	template <typename... Args>
	using binding_info_ptr = std::unique_ptr<binding_info_t<Args...>>;

}

// event-backend
namespace atma::detail
{
	using base_binding_iters_t = std::vector<base_binding_infos_t::iterator>;

	struct per_system_bindings_t
	{
		per_system_bindings_t(event_system_backend_t* event_system_backend)
			: event_system_backend{event_system_backend}
		{}

		event_system_backend_t* event_system_backend = nullptr;
		base_binding_iters_t binding_iters;
	};

	using per_system_bindings_list_t = std::vector<per_system_bindings_t>;


	struct event_backend_t
		: atma::ref_counted
	{
		per_system_bindings_list_t bound_systems;
		std::mutex bound_systems_mutex;

		template <typename... Args, typename F>
		void bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f);

		template <typename... Args>
		void raise(Args...);
	};

}


// event-system-backend
namespace atma::detail
{
	struct event_system_backend_t
	{
		using queues_t = std::map<thread_id_t, lockfree_queue_t>;

		event_system_backend_t();
		event_system_backend_t(event_system_backend_t const&) = delete;

		template <typename... Args, typename F>
		auto bind(event_backend_ptr const& event_backend, base_binding_iters_t& event_backend_bindings, thread_id_t thread_id, F&& f) -> void;

		template <typename... Args>
		auto raise(base_binding_iters_t const& iters, thread_id_t thread_id, Args... args) -> void;

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
		std::vector<event_backend_ptr> event_backends_;

		friend struct event_backend_t;
	};


}



// event-system-backend implementation
namespace atma::detail
{
	inline event_system_backend_t::event_system_backend_t()
		: threaded_args_resource{16, 64, 32}
	{}

	template <typename... Args, typename F>
	inline auto event_system_backend_t::bind(event_backend_ptr const& event_backend, base_binding_iters_t& event_backend_bindings, thread_id_t thread_id, F&& f) -> void
	{
		ATMA_SCOPED_LOCK(bindings_mutex_)
		{
			// register the event-backend with us
			if (auto candidate = atma::find_in(event_backends_, event_backend); candidate == event_backends_.end())
				event_backends_.push_back(event_backend);

			// store the new binding
			auto new_iter = bindings_.insert(bindings_.end(), std::make_unique<binding_info_t<Args...>>(std::forward<F>(f), thread_id));

			// tell the event-backend which index into our binding 
			event_backend_bindings.push_back(new_iter);
		}
	}

	template <typename... Args>
	inline auto event_system_backend_t::raise(base_binding_iters_t const& iters, thread_id_t thread_id, Args... args) -> void
	{
		using tuple_type = std::tuple<Args...>;

		auto this_thread_id = std::this_thread::get_id();

		// no thread specified, free to schedule as normal
		if (thread_id == thread_id_t{})
		{
			std::map<thread_id_t, tuple_type*> argument_store;

			// first, queue all tethered bindings
			for (auto&& iter : iters)
			{
				binding_info_t<Args...> const& binding_info = *static_cast<binding_info_t<Args...>*>(iter->get());

				// filter out wandering bindings, and tethered bindings to this thread (they will be executed immediately)
				if (binding_info.thread_id == thread_id_t{} || binding_info.thread_id == this_thread_id)
					continue;

				// create a copy of the arguments for the requested thread (if we haven't already)
				auto candidate = argument_store.find(binding_info.thread_id);
				if (candidate == argument_store.end())
				{
					auto tuple_args_ptr = (tuple_type*)threaded_args_resource.allocate(sizeof(tuple_type));
					new (tuple_args_ptr) tuple_type{args...};
					auto[riter, r] = argument_store.insert({binding_info.thread_id, tuple_args_ptr});
					ATMA_ASSERT(r);
					candidate = riter;
				}

				// using the per-thread copy of the arguments, enqueue the command
				auto& tupled_arguments = *candidate->second;
				atma::enqueue_function_to_queue(
					per_thread_queue(binding_info.thread_id),
					std::function<void()>{[f = binding_info.f, &tupled_arguments]{std::apply(f, tupled_arguments); }});
			}

			// enqueue the deletion of the per-thread argument copies
			for (auto const&[arg_thread_id, tuple_args_ptr] : argument_store)
			{
				atma::enqueue_function_to_queue(
					per_thread_queue(arg_thread_id),
					[&, tuple_args_ptr] { threaded_args_resource.deallocate(tuple_args_ptr, sizeof(tuple_type)); });
			}

			// execute the rest immediately
			for (auto&& iter : iters)
			{
				binding_info_t<Args...> const& binding_info = *static_cast<binding_info_t<Args...>*>(iter->get());

				if (binding_info.thread_id == thread_id_t{} || binding_info.thread_id == this_thread_id)
				{
					std::invoke(binding_info.f, args...);
				}
			}
		}
		// thread specified, we need to exclude tethered bindings and "move" thread-wandering bindings
		else
		{
			for (auto&& iter : iters)
			{
				binding_info_t<Args...> const& binding_info = *static_cast<binding_info_t<Args...>*>(iter->get());

				// thread-tethered binding for this very thread, run immediately
				if (binding_info.thread_id == thread_id && thread_id == this_thread_id)
				{
					std::invoke(binding_info.f, args...);
				}
				// thread-tethered binding for another thread, or thread-wandering binding, enqueue for the requested thread
				else if (binding_info.thread_id == thread_id || binding_info.thread_id == thread_id_t{})
				{
					atma::enqueue_function_to_queue(
						per_thread_queue(thread_id),
						std::function<void()>{atma::bind(binding_info.f, args...)});
				}
			}
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
		auto& esb = event_system.backend();

		ATMA_SCOPED_LOCK(bound_systems_mutex)
		{
			auto candidate = std::find_if(bound_systems.begin(), bound_systems.end(),
				[&esb](auto&& x) { return x.event_system_backend == &esb; });

			if (candidate == bound_systems.end())
				candidate = bound_systems.emplace(bound_systems.end(), &event_system.backend());

			esb.bind<Args...>(shared_from_this<event_backend_t>(), candidate->binding_iters, thread_id, std::forward<F>(f));
		}
	}

	template <typename... Args>
	inline void event_backend_t::raise(Args... args)
	{
		ATMA_SCOPED_LOCK(bound_systems_mutex)
		{
			for (auto&& system : bound_systems)
			{
				if (!system.event_system_backend)
					continue;

				system.event_system_backend->raise<Args...>(system.binding_iters, thread_id_t{}, std::forward<Args>(args)...);
			}
		}
	}
}


























namespace atma::detail
{
	extern event_system_t default_event_system;
	extern event_binder_t default_event_binder;
}

namespace atma
{
	struct event_system_t : flyweight_t<detail::event_system_backend_t>
	{
		template <typename F, typename... Args>
		auto bind(event_t<Args...>& e, event_binder_t& b, F&& f) -> void
			{ backend().bind(e, b, std::forward<F>(f), std::this_thread::get_id()); }

		template <typename F, typename... Args>
		auto bind(event_t<Args...>& e, event_binder_t& b, thread_id_t thread_id, F&& f) -> void
			{ backend().bind(e.backend_, b, thread_id, std::forward<F>(f)); }

		template <typename... EArgs>
		auto raise(event_t<EArgs...>& e, EArgs... args) -> void
			{ backend().raise(e, args...); }

		auto process_events_for_this_thread() -> void
			{ backend().process_events_for(std::this_thread::get_id()); }

		friend struct detail::event_backend_t;
	};

	template <typename F>
	struct bound_event_t
	{
		event_binder_t& b;
		F f;
	};


	template <typename... Args>
	struct event_t
	{
		event_t()
		{
			backend_ = detail::event_backend_ptr::make();
		}

		template <typename... RArgs>
		auto raise(RArgs&&... args) -> void
		{
			if (!backend_)
				return;

			backend_->raise<Args...>(std::forward<RArgs>(args)...);
		}

		template <typename F>
		void bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f)
		{
			backend_->bind<Args...>(event_system, binder, thread_id, std::forward<F>(f));
		}

		template <typename F>
		void bind(F&& f)
		{
			this->bind(detail::default_event_system, detail::default_event_binder, std::this_thread::get_id(), std::forward<F>(f));
		}

		template <typename F>
		void bind(F&& f, thread_id_t id)
		{
			detail::default_event_system.bind(*this, detail::default_event_binder, std::forward<F>(f), id);
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
		detail::event_backend_ptr backend_;

		friend struct ::atma::event_system_t;
		friend struct ::atma::detail::event_system_backend_t;
	};

	struct event_binder_t
	{
		auto is_bound() const -> bool;
		
		template <typename F>
		auto operator + (F&& f) -> bound_event_t<F>
		{
			return {*this, f};
		}

	private:
		
	};


}

namespace atma::detail
{
	inline event_system_t default_event_system;
	inline event_binder_t default_event_binder;
}