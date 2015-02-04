#if 0
#pragma once

#include <atma/assert.hpp>
#include <atma/lockfree/queue.hpp>
#include <atma/function.hpp>
#include <mutex>
#include <functional>
#include <vector>
#include <set>

namespace atma {

	//=====================================================================
	//=====================================================================
	namespace detail
	{
		template <typename FN, typename R, typename... Args>
		struct wrap_event_fn_t
		{
			static auto wrap(FN const& fn) -> std::function<atma::event_flow_t(Args...)>
			{
				return {fn};
			}
		};

		template <typename FN, typename... Args>
		struct wrap_event_fn_t<FN, void, Args...>
		{
			static auto wrap(FN const& fn) -> std::function<atma::event_flow_t(Args...)>
			{
				return [&fn](Args const&... args) -> atma::event_flow_t {
					fn(args...);
					return atma::event_flow_t();
				};
			}
		};

		template <typename FN, typename... Args>
		inline auto wrap_event_fn(FN const& fn)->std::function<atma::event_flow_t(Args...)>
		{
			return wrap_event_fn_t<FN, typename atma::function_traits<FN>::result_type, Args...>
			  ::wrap(fn);
		}
	}

	

	//=====================================================================
	// event_t
	//=====================================================================
	template <typename FN>
	struct event_t
	{
		using result_type = typename function_traits<FN>::reuslt_type;
		
		// fire
		template <typename... Args>
		auto fire(Args&&... args) -> result_type
		{
			std::lock_guard<std::mutex> LG(mutex_);

			event_flow_t fc;
			for (auto const& x : delegates_)
			{
				if (fc.allow(std::get<0>(x)))
					fc += std::get<1>(x)(std::forward<Args>(args)...);
			}

			return fc;
		}


	private:
		std::mutex mutex_;
		typedef std::tuple<std::string, delegate_t> bound_delegate_t;

		struct comp_t
		{
			auto operator ()(bound_delegate_t const& lhs, bound_delegate_t const& rhs) -> bool
			{
				return std::get<0>(lhs) < std::get<0>(rhs);
			}
		};
		

		typedef std::set<bound_delegate_t, comp_t> bound_delegates_t;
		bound_delegates_t delegates_;
	};

}
#endif