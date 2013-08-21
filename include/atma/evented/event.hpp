//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_EVENTED_EVENT_HPP
#define ATMA_EVENTED_EVENT_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atma/evented/delegate.hpp>
#include <atomic>
#include <functional>
#include <vector>
//=====================================================================
namespace atma {
namespace evented {
//=====================================================================
	
	//=====================================================================
	// event_t
	//=====================================================================
	template <typename F = typename std::identity<void()>::type>
	struct event_t
	{
		typedef std::function<F> delegate_t;

		auto operator += (delegate_t const& t) -> void {
			connect(t);
		}

		// connect
		auto connect(delegate_t const& t) -> void {
			delegates_.push_back(t);
		}

		// fire
		template <typename... Args>
		auto fire(Args&&... args) -> void
		{
			for (auto const& x : delegates_) {
				x(std::forward<Args>(args)...);
			}
		}


	private:
		typedef std::vector<delegate_t> delegates_t;
		delegates_t delegates_;
	};



//=====================================================================
} // namespace evented
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
