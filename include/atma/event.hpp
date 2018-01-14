#pragma once

#include <atma/lockfree_queue.hpp>
#include <atma/flyweight.hpp>
#include <atma/function.hpp>

#include <functional>
#include <memory>
#include <unordered_map>

namespace atma
{
	template <typename... Args>
	struct event_t;

	struct event_binder_t;

	struct event_system_t;
}


namespace atma::detail
{
	struct base_binding_info_t
	{
		virtual ~base_binding_info_t() {}
	};

	template <typename... Args>
	struct binding_info_t : base_binding_info_t
	{
		template <typename F>
		binding_info_t(F&& f)
			: f{std::forward<F>(f)}
		{}

		~binding_info_t() override = default;

		atma::function<void(Args...)> f;
	};
	
	template <typename... Args>
	using binding_info_ptr = std::unique_ptr<binding_info_t<Args...>>;



	struct event_system_backend_t : std::enable_shared_from_this<event_system_backend_t>
	{
		template <typename F, typename... Args>
		auto bind(event_t<Args...>& e, event_binder_t& b, F&& f) -> void
		{
			if (!e.info_)
			{
				auto [iter, r] = infos_.insert(std::make_unique<binding_info_t<Args...>>(std::forward<F>(f)));
				ATMA_ASSERT(r);
				e.es_ = weak_from_this();
				e.info_ = static_cast<binding_info_t<Args...>*>(iter->get());
			}
		}

		template <typename... Args>
		auto raise(event_t<Args...>& e, Args... args) -> void
		{
			if (!e.info_)
				return;

			std::invoke(e.info_->f, args...);
		}

	private:
		uint next_event_id_ = 0;
		std::set<std::unique_ptr<base_binding_info_t>> infos_;
		lockfree_queue_t<false> queue_;
	};

	using event_system_backend_wptr = std::weak_ptr<event_system_backend_t>;
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
			{ backend().bind(e, b, std::forward<F>(f)); }

		template <typename... EArgs>
		auto raise(event_t<EArgs...>& e, EArgs... args) -> void
			{ backend().raise(e, args...); }
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
			if (auto es = es_.lock())
				es->raise(*this, std::forward<RArgs>(args)...);
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
		detail::event_system_backend_wptr es_;
		detail::binding_info_t<Args...>* info_ = nullptr;

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