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
	
	struct flowcontrol_t
	{
		flowcontrol_t()
			: break_(), prevent_default_()
		{
		}

		auto stop_execution() -> void { break_ = true; }
		auto prevent_default_behaviour() -> void { prevent_default_ = true; }

		auto broke() const -> bool { return break_; }
		auto prevent_default() const -> bool { return prevent_default_; }

	private:
		bool break_;
		bool prevent_default_;
	};



	//=====================================================================
	// event_t
	//=====================================================================
	template <typename... Args>
	struct event_t
	{
		

		typedef std::function<void(flowcontrol_t&, Args...)> delegate_t;

		auto operator += (delegate_t const& t) -> void {
			connect(t);
		}

		// connect
		auto connect(delegate_t const& t) -> void {
			delegates_.push_back(t);
		}

		// fire
		template <typename... Args>
		auto fire(Args&&... args) -> flowcontrol_t
		{
			flowcontrol_t fc;
			for (auto const& x : delegates_) {
				x(fc, std::forward<Args>(args)...);
				if (fc.broke())
					break;
			}

			return fc;
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
