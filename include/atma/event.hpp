#pragma once

#include <atma/event.hpp>
#include <functional>
#include <list>
#include <set>
#include <mutex>


namespace atma
{
	template <typename T>
	struct null_combiner_t
	{
		null_combiner_t()
			: result_()
		{
		}

		using result_type = T;

		auto reset() -> void { result_ = T(); }
		auto push(T const& x) -> void { result_ += x; }
		auto result() -> T { return result_; }

	private:
		T result_;
	};

	template <>
	struct null_combiner_t<void>
	{
		null_combiner_t()
		{
		}

		using result_type = void;

		auto reset() -> void {}
		auto push(void) -> void {}
		auto result() const -> void { return; }
	};

	template <typename T>
	struct count_combiner_t
	{
		count_combiner_t()
			: calls_()
		{
		}

		using result_type = uint;

		auto reset() -> void { calls_ = 0u; }
		template <typename Y> auto push(Y const&) -> void { ++calls_; }
		auto push(void) -> void { ++calls_; }
		auto result() -> uint { return calls_; }

	private:
		uint calls_;
	};

	namespace detail
	{
		template <typename FN>
		using delegate_t = std::function<FN>;

		template <typename FN>
		using delegates_t = std::list<delegate_t<FN>>;



		template <typename R, template <typename> class CMB, typename FN, typename... Args>
		struct fire_impl_t
		{
			static auto go(CMB<R>& cmb, delegates_t<FN> const& delegates, Args&&... args) -> typename CMB<R>::result_type
			{
				cmb.reset();

				for (auto const& x : delegates)
					cmb.push(x(std::forward<Args>(args)...));

				return cmb.result();
			}
		};


		template <template <typename> class CMB, typename FN, typename... Args>
		struct fire_impl_t<void, CMB, FN, Args...>
		{
			static auto go(CMB<void>& cmb, delegates_t<FN> const& delegates, Args&&... args) -> typename CMB<void>::result_type
			{
				cmb.reset();

				for (auto const& x : delegates) {
					x(std::forward<Args>(args)...);
					cmb.push();
				}

				return cmb.result();
			}
		};

	}

	template <bool Use> struct maybe_mutex_t;
	template <bool Use> struct maybe_scoped_lock_t;

	template <>
	struct maybe_mutex_t<true>
	{
		maybe_mutex_t() {}
		maybe_mutex_t(maybe_mutex_t const&) = delete;

	private:
		std::mutex mutex;

		friend struct maybe_scoped_lock_t<true>;
	};

	template <>
	struct maybe_mutex_t<false>
	{
		maybe_mutex_t(maybe_mutex_t const&) = delete;
	};

	template <>
	struct maybe_scoped_lock_t<true>
	{
		maybe_scoped_lock_t(maybe_mutex_t<true>& mtx)
			: guard_(mtx.mutex)
		{
		}

	private:
		std::lock_guard<std::mutex> guard_;
	};

	template <>
	struct maybe_scoped_lock_t<false>
	{
		maybe_scoped_lock_t(maybe_mutex_t<false> const& mtx)
		{
		}
	};


	template <typename FN, template <typename> class CMB = count_combiner_t, bool ThreadSafe = true>
	struct event_t
	{
		using fnptr_type = std::decay_t<FN>;
		using fn_result_type = typename atma::function_traits<fnptr_type>::result_type;

		using delegate_t = std::function<FN>;
		using delegates_t = detail::delegates_t<FN>;
		using delegate_handle_t = typename delegates_t::const_iterator;

		using combiner_t = CMB<fn_result_type>;
		using result_type = typename combiner_t::result_type;


		event_t()
		{
		}

		event_t(combiner_t const& cmb)
			: combiner_{cmb}
		{
		}

		event_t(event_t const& rhs)
			: combiner_{rhs.combiner_}, mutex_{}, delegates_(rhs.delegates_)
		{}

		template <typename... Args>
		auto fire(Args&&... args) -> result_type
		{
			auto SL = maybe_scoped_lock_t<ThreadSafe>{mutex_};

			return detail::fire_impl_t<fn_result_type, CMB, FN, Args...>::go(combiner_, delegates_, std::forward<Args>(args)...);
		}

		auto operator += (delegate_t const& fn) -> delegate_handle_t
		{
			auto SL = maybe_scoped_lock_t<ThreadSafe>{mutex_};

			delegates_.push_back(fn);
			return --delegates_.end();
		}

		auto operator -= (delegate_handle_t const& fn) -> void
		{
			auto SL = maybe_scoped_lock_t<ThreadSafe>{mutex_};

			delegates_.erase(fn);
		}

	private:


	private:
		combiner_t combiner_;
		delegates_t delegates_;
		maybe_mutex_t<ThreadSafe> mutex_;
	};


	template <typename Range, typename C, typename M, typename T>
	struct propper_t;

	template <typename Range, typename C, typename M, typename... Args>
	struct propper_t<Range, C, M, std::tuple<Args...>>
	{
		static auto go(Range&& range, M C::*member, Args... args) -> typename M::result_type
		{
			for (auto&& x : range)
				(x.*member).fire(std::forward<Args>(args)...);

			return typename M::result_type();
		}
	};

	template <typename Range, typename C, typename M>
	auto broadcast_across(Range&& range, M C::*member)
	-> typename M::delegate_t
	{
		using params = typename atma::function_traits<typename M::fnptr_type>::tupled_args_type;

		return atma::curry(&propper_t<Range, C, M, params>::go, std::ref(range), member);
	}
}

