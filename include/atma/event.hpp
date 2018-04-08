#pragma once

#include <atma/lockfree_queue.hpp>
#include <atma/flyweight.hpp>
#include <atma/function.hpp>
#include <atma/threading.hpp>
#include <atma/algorithm.hpp>

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
	using event_system_backend_wptr = std::weak_ptr<event_system_backend_t>;

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

		template <typename... Args, typename F>
		void bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f);

		template <typename... Args>
		void raise(Args...);
	};

}


// event-system-backend
namespace atma::detail
{
	struct event_system_backend_t : std::enable_shared_from_this<event_system_backend_t>
	{
		using queues_t = std::map<thread_id_t, lockfree_queue_t>;

		event_system_backend_t()
		{}

		template <typename... Args, typename F>
		auto bind(event_backend_ptr const& event_backend, int sys_bind_idx, thread_id_t thread_id, F&& f) -> void
		{
			// register the event-backend with us
			if (auto candidate = atma::find_in(event_backends_, event_backend); candidate == event_backends_.end())
				event_backends_.push_back(event_backend);

			// store the new binding
			auto new_iter = bindings_.insert(bindings_.end(), std::make_unique<binding_info_t<Args...>>(std::forward<F>(f), thread_id));

			// tell the event-backend which index into our binding 
			event_backend->bound_systems[sys_bind_idx].binding_iters.push_back(new_iter);
		}

		template <typename... Args>
		auto raise(base_binding_iters_t const& iters, thread_id_t thread_id, Args... args) -> void
		{
			// no thread specified, free to schedule as normal
			if (thread_id == thread_id_t{})
			{
				std::map<thread_id_t, uint> argument_store;

				for (auto&& iter : iters)
				{
					base_binding_info_ptr const& bbi = *iter;
					binding_info_t<Args...> const& binding_info = *static_cast<binding_info_t<Args...>*>(bbi.get());
					
					if (binding_info.thread_id == std::this_thread::get_id())
					{
						std::invoke(binding_info.f, args...);
					}
					else
					{
						auto& tupled_arguments = per_thread_argument_store(binding_info.thread_id, argument_store[binding_info.thread_id], std::forward<Args>(args)...);

						atma::enqueue_function_to_queue(
							per_thread_queue(binding_info.thread_id),
							std::function<void()>{[f=binding_info.f, &tupled_arguments] { std::apply(f, tupled_arguments); }});
					}
				}

				// delete the temporary per-thread argument storage
				for (auto const& [thread_id, idx] : argument_store)
				{
					atma::enqueue_function_to_queue(
						per_thread_queue(thread_id),
						[&]{ argument_store_[thread_id].ptrs.erase(idx); });
				}
			}
			// thread specified, we need to exclude thread-settled bindings and "move" thread-wandering bindings
			else
			{
				for (auto&& iter : iters)
				{
					base_binding_info_ptr const& bbi = *iter;
					binding_info_t<Args...> const& binding_info = *static_cast<binding_info_t<Args...>*>(bbi.get());

					if (binding_info.thread_id == std::this_thread::get_id())
					{
						std::invoke(binding_info.f, args...);
					}
					else
					{
						atma::enqueue_function_to_queue(
							per_thread_queue(binding_info.thread_id),
							std::function<void()>{atma::bind(binding_info.f, args...)});
					}
				}
			}
			
		}

		auto process_events_for(thread_id_t thread_id) -> void
		{
			auto candidate = queues_.find(thread_id);
			if (candidate == queues_.end())
				return;

			atma::consume_queue_of_functions<>(candidate->second);
		}

		template <typename... Args>
		decltype(auto) per_thread_argument_store(thread_id_t thread_id, uint& idx, Args... args)
		{
			if (idx != 0)
				return *reinterpret_cast<std::tuple<Args...>*>(argument_store_[thread_id].ptrs[idx].get());

			using tuple_type = std::tuple<Args...>;

			auto& store = argument_store_[thread_id];
			auto& our_store = store.ptrs[store.current_idx++];
			our_store.reset(new char[sizeof(tuple_type)]);
			new (our_store.get()) std::tuple<Args...>{args...};
			return *reinterpret_cast<std::tuple<Args...>*>(our_store.get());
		}
	

	private:
		auto per_thread_queue(thread_id_t id) -> lockfree_queue_t&
		{
			auto candidate = queues_.find(id);
			if (candidate == queues_.end())
			{
				auto [iter, success] = queues_.emplace(id, 512);
				ATMA_ASSERT(success);
				candidate = iter;
			}

			return candidate->second;
		}

	private:
		queues_t queues_;
		base_binding_infos_t bindings_;

		using per_thread_store_t = struct { uint current_idx = 0; std::map<uint, std::unique_ptr<char[]>> ptrs; };
		std::map<thread_id_t, per_thread_store_t> argument_store_;

		// these are the event-backends that have bindings through us
		std::vector<event_backend_ptr> event_backends_;

		friend struct event_backend_t;
	};


}


// event-backend implementation
namespace atma::detail
{
	template <typename... Args, typename F>
	inline void event_backend_t::bind(event_system_t& event_system, event_binder_t& binder, thread_id_t thread_id, F&& f)
	{
		auto& esb = event_system.backend();

		auto candidate = std::find_if(bound_systems.begin(), bound_systems.end(),
			[&esb](auto&& x) { return x.event_system_backend == &esb; });

		if (candidate == bound_systems.end())
			candidate = bound_systems.emplace(bound_systems.end(), &event_system.backend());
		
		esb.bind<Args...>(shared_from_this<event_backend_t>(), (int)std::distance(bound_systems.begin(), candidate), thread_id, std::forward<F>(f));
	}

	template <typename... Args>
	inline void event_backend_t::raise(Args... args)
	{
		for (auto&& system : bound_systems)
		{
			if (!system.event_system_backend)
				continue;

			system.event_system_backend->raise<Args...>(system.binding_iters, thread_id_t{}, std::forward<Args>(args)...);
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
			if (!backend_)
				backend_ = detail::event_backend_ptr::make();

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