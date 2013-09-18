//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_EVENTED_EVENT_HPP
#define ATMA_EVENTED_EVENT_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atma/lockfree/queue.hpp>
#include <atomic>
#include <functional>
#include <vector>
//=====================================================================
namespace atma {
//=====================================================================
	
	//=====================================================================
	// event_flow_t
	//=====================================================================
	struct event_flow_t
	{
		event_flow_t()
			: break_(), prevent_default_()
		{
		}

		auto stop_execution() -> void { break_ = true; }
		auto prevent_default_behaviour() -> void { prevent_default_ = true; }

		auto is_broken() const -> bool { return break_; }
		auto is_prevented() const -> bool { return prevent_default_; }

	private:
		bool break_;
		bool prevent_default_;
	};

#if 0
	struct event_handle_t
	{
	private:
		event_handle_t(void* ptr)
			: ptr_(ptr)
		{
		}

		void* ptr_;

		template <typename... Args>
		friend struct event_t;

		friend auto operator == (event_handle_t const& lhs, event_handle_t const& rhs) -> bool;
	};

	inline auto operator == (event_handle_t const& lhs, event_handle_t const& rhs) -> bool
	{
		return lhs.ptr() == rhs.ptr();
	}
#endif

	//=====================================================================
	// event_t
	//=====================================================================
	template <typename... Args>
	struct event_t
	{
		typedef std::function<void(event_flow_t&, Args...)> delegate_t;
		typedef typename atma::lockfree::queue_t<delegate_t>::iterator delegate_handle_t;

		// connect
		auto connect(delegate_t const& t) -> delegate_handle_t
		{
			return delegates_.push(t);
		}

		auto operator += (delegate_t const& t) -> void
		{
			connect(t);
		}

		// disconnect
		auto disconnect(delegate_handle_t handle) -> void
		{
			delegates_.erase(handle);
		}

		// fire
		template <typename... Args>
		auto fire(Args&&... args) -> event_flow_t
		{
			event_flow_t fc;
			for (auto const& x : delegates_) {
				x(fc, std::forward<Args>(args)...);
				if (fc.is_broken())
					break;
			}

			return fc;
		}


	private:
		typedef atma::lockfree::queue_t<delegate_t> delegates_t;
		delegates_t delegates_;
	};

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
