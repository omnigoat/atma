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
//=====================================================================
namespace atma {
namespace evented {
//=====================================================================
	
	
	//=====================================================================
	// event_t
	//=====================================================================
	template <typename R = void, typename... Params>
	struct event_t
	{
		template <typename T>
		auto operator += (T const& t) -> void {
			connect(t);
		}

		// connect
		template <typename T>
		auto connect(T const& t) -> void {
			delegates_.push_back(make_delegate(t));
		}

		// fire
		auto fire(Params&&... params) -> void
		{
			for (auto const& x : delegates_) {
				x->operator ()(std::move<Params>(params)...);
			}
		}


	private:
		typedef std::vector<intrusive_ptr<delegate_t<R, Params...>>> delegates_t;
		delegates_t delegates_;
	};



//=====================================================================
} // namespace evented
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
