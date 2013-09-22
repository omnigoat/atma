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
#include <atma/xtm/function.hpp>
#include <mutex>
#include <functional>
#include <vector>
#include <set>
//=====================================================================
namespace atma {
//=====================================================================
	
	namespace detail
	{
		namespace idclass {
			inline auto id(std::string const& x) -> std::pair<std::string::const_iterator, std::string::const_iterator>
			{
				auto i = x.begin();
				for (; i != x.end(); ++i)
					if (*i == '.')
						break;
				return {x.begin(), i};
			}

			inline auto classes(std::string const& x) -> std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>>
			{
				std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>> result;

				auto c0 = x.find_first_of('.', 0);
				while (c0 != std::string::npos) {
					auto c1 = x.find_first_of('.', c0);
					if (c1 == std::string::npos)
						c1 = x.size() - 1;
					result.push_back({x.begin() + c0, x.begin() + c1});
					c0 = x.find_first_of('.', c1);
				}

				return result;
			}
		}
	}

	//=====================================================================
	// event_flow_t
	//=====================================================================
	struct event_flow_t
	{
		event_flow_t()
			: propagating_(true)
		{
		}

		auto stop_propagation() -> void {
			propagating_ = true;
		}

		auto is_propagating() const -> bool
		{
			return propagating_;
		}

		auto prevent_default_behaviour() -> void { 
			disallowed_.insert("default");
		}

		auto disallow(std::string const& x) -> void {
			disallowed_.insert(x);
		}

		auto allow(std::string const& x) -> bool
		{
			return disallowed_.find(x) == disallowed_.end();
		}

		auto operator += (event_flow_t const& rhs) -> void
		{
			propagating_ = propagating_ && rhs.is_propagating();
			disallowed_.insert(rhs.disallowed_.begin(), rhs.disallowed_.end());
		}

	private:
		std::set<std::string> disallowed_;
		
		bool propagating_;
	};



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
			return wrap_event_fn_t<FN, typename atma::xtm::function_traits<FN>::result_type, Args...>
			  ::wrap(fn);
		}
	}

	

	//=====================================================================
	// event_t
	//=====================================================================
	template <typename... Args>
	struct event_t
	{
		typedef std::function<atma::event_flow_t(Args...)> delegate_t;
		//typedef typename atma::lockfree::queue_t<delegate_t>::iterator delegate_handle_t;

		// add
		auto add(std::string const& name, delegate_t const& t) -> void
		{
			std::lock_guard<std::mutex> LG(mutex_);
			delegates_.insert({name, t});
		}

		template <typename FN>
		auto add(std::string const& name, FN const& fn) -> void
		{
			std::lock_guard<std::mutex> LG(mutex_);
			delegates_.insert(std::make_tuple(name, detail::wrap_event_fn<FN, Args...>(fn)));
		}

#if 0
		auto operator += (delegate_t const& t) -> void
		{
			connect(t);
		}

		// disconnect
		auto disconnect(delegate_handle_t handle) -> void
		{
			delegates_.erase(handle);
		}
#endif


		// fire
		template <typename... Args>
		auto fire(Args&&... args) -> event_flow_t
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

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
