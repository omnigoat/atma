#pragma once

#include <atma/tuple.hpp>
#include <tuple>

namespace atma
{
	template <typename> struct function;




	// functor-buf
	namespace detail
	{
		// size of buffer to store functor. make this larger will allow for more
		// data to be stored in the "Small Function Optimization", but each instance
		// of function will take up more space.
		uint const bufsize_bytes = 32 - sizeof(void*) - sizeof(void*);

		struct functor_buf_t
		{
			char pad[bufsize_bytes];
		};
	}




	// vtable-impl
	namespace detail
	{
		template <typename FN>
		struct small_functor_optimization_t
		{
			static bool const value = sizeof(FN) <= bufsize_bytes;
		};

		template <typename FN, bool = small_functor_optimization_t<FN>::value>
		struct vtable_impl_t
		{
			static auto store(functor_buf_t& buf, FN const& fn) -> void
			{
				new (&buf) FN{fn};
			}

			static auto destruct(functor_buf_t& fn) -> void
			{
				reinterpret_cast<FN&>(fn).~FN();
			}

			static auto copy(functor_buf_t& dest, functor_buf_t const& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN const&>(src);
				new (&dest) FN{rhs};
			}

			static auto move(functor_buf_t& dest, functor_buf_t&& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN&&>(src);
				new (&dest) FN{std::move(rhs)};
			}

			template <typename R, typename... Args>
			static auto call(functor_buf_t const& buf, Args... args) -> R
			{
				auto& fn = reinterpret_cast<FN&>(const_cast<functor_buf_t&>(buf));
				return fn(std::forward<Args>(args)...);
			}

			static auto target(functor_buf_t& buf) -> void*
			{
				auto&& fn = reinterpret_cast<FN&>(buf);
				return &fn;
			}
		};

		template <typename FN>
		struct vtable_impl_t<FN, false>
		{
			static auto store(functor_buf_t& buf, FN const& fn) -> void
			{
				reinterpret_cast<FN*&>(buf) = new FN(fn);
			}

			static auto move(functor_buf_t& dest, functor_buf_t&& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN*&>(src);
				auto&& lhs = reinterpret_cast<FN*&>(dest);
				lhs = new FN{std::move(*rhs)};
			}

			static auto copy(functor_buf_t& dest, functor_buf_t const& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN* const&>(src);
				auto&& lhs = reinterpret_cast<FN*&>(dest);
				lhs = new FN{std::move(*rhs)};
			}

			static auto destruct(functor_buf_t& buf) -> void
			{
				auto&& tbuf = reinterpret_cast<FN*&>(buf);
				delete tbuf;
			}

			template <typename R, typename... Args>
			static auto call(functor_buf_t const& buf, Args... args) -> R
			{
				return (*reinterpret_cast<FN* const&>(buf))(std::forward<Args>(args)...);
			}

			static auto target(functor_buf_t& buf) -> void*
			{
				auto&& ptr = reinterpret_cast<FN*&>(buf);
				return ptr;
			}
		};
	}




	// vtable
	namespace detail
	{
		template <typename R, typename... Params>
		struct functor_vtable_t
		{
			using destruct_fntype = auto(*)(detail::functor_buf_t&) -> void;
			using copy_fntype     = auto(*)(detail::functor_buf_t&, detail::functor_buf_t const&) -> void;
			using move_fntype     = auto(*)(detail::functor_buf_t&, detail::functor_buf_t&&) -> void;
			using target_fntype   = auto(*)(detail::functor_buf_t&) -> void*;

			destruct_fntype destruct;
			copy_fntype     copy;
			move_fntype     move;
			target_fntype   target;
		};

		template <typename FN, typename R, typename... Params>
		auto generate_vtable() -> functor_vtable_t<R, Params...>*
		{
			static auto _ = functor_vtable_t<R, Params...>
			{
				&vtable_impl_t<FN>::destruct,
				&vtable_impl_t<FN>::copy,
				&vtable_impl_t<FN>::move,
				&vtable_impl_t<FN>::target,
			};

			return &_;
		}
	}




	// functor-wrapper
	namespace detail
	{
		template <typename R, typename... Params>
		struct functor_wrapper_t
		{
			functor_wrapper_t()
				: vtable()
				, buf{}
			{}

			template <typename FN>
			explicit functor_wrapper_t(FN&& fn)
				: vtable{generate_vtable<FN, R, Params...>()}
				, buf{}
			{
				vtable_impl_t<FN>::store(buf, std::forward<FN>(fn));
			}

			functor_wrapper_t(functor_wrapper_t const& rhs)
				: vtable{rhs.vtable}
				, buf{}
			{
				vtable->copy(buf, rhs.buf);
			}

			functor_wrapper_t(functor_wrapper_t&& rhs)
				: vtable{rhs.vtable}
				, buf{}
			{
				vtable->move(buf, std::move(rhs.buf));
			}

			~functor_wrapper_t()
			{
				vtable->destruct(buf);
			}

			auto operator = (functor_wrapper_t const& rhs) -> functor_wrapper_t&
			{
				vtable->destruct(buf);
				vtable = rhs.vtable;
				vtable->copy(buf, rhs.buf);
				return *this;
			}

			auto operator = (functor_wrapper_t&& rhs) -> functor_wrapper_t&
			{
				vtable->destruct(buf);
				vtable = rhs.vtable;
				vtable->move(buf, std::move(rhs.buf));
				return *this;
			}

			template <typename T>
			auto target() const -> T const*
			{
				return reinterpret_cast<T const*>(vtable->target(const_cast<detail::functor_buf_t&>(buf)));
			}

			auto move_into(functor_wrapper_t& rhs) -> void
			{
				vtable->move(rhs.buf, std::move(buf));
				rhs.vtable = vtable;

				vtable->destruct(buf);
			}

			functor_vtable_t<R, Params...> const* vtable;
			functor_buf_t buf;
		};
	}




	// dispatch
	namespace detail
	{
		template <typename R, typename... Params>
		using dispatch_fnptr_t = auto(*)(functor_buf_t const&, Params...) -> R;


		template <typename R, typename Params, typename Args, size_t I>                   struct dispatcher_t;
		template <typename R, typename Params, typename Args, size_t I, bool LT, bool GT> struct dispatcher_ii_t;
		template <typename R, typename Params, typename UParams, typename RParams>        struct dispatcher_iii_t;

		template <typename R, typename... Params, typename... UParams, typename... RParams>
		struct dispatcher_iii_t<R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>
		{
			static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, UParams...) -> function<R(RParams...)>;
		};

		// superfluous arguments
		template <typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_ii_t<R, std::tuple<Params...>, Args, I, false, true>
		{
			template <typename... More>
			static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Params..., More&&...) -> R;
		};

		// partial-function
		template <typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_ii_t<R, std::tuple<Params...>, Args, I, true, false>
			: dispatcher_iii_t<R, std::tuple<Params...>,
				tuple_select_t<idxs_list_t<I>, std::tuple<Params...>>,
				tuple_select_t<idxs_range_t<I, sizeof...(Params)>, std::tuple<Params...>>>
		{
		};

		// correct arguments
		template <typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_ii_t<R, std::tuple<Params...>, Args, I, false, false>
		{
			static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Params...) -> R;
		};

		template <typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_t<R, std::tuple<Params...>, Args, I>
			: dispatcher_ii_t<R, std::tuple<Params...>, Args, I, (I < sizeof...(Params)), (sizeof...(Params) < I)>
		{
		};
	}




	// functorize
	namespace detail
	{
		template <typename T>
		auto functorize(T&& t) -> T
		{
			return t;
		}

		template <typename R, typename C, typename... Params>
		auto functorize(R(C::*fn)(Params...)) -> decltype(std::mem_fn(fn))
		{
			return std::mem_fn(fn);
		}

		template <typename R, typename C, typename... Params>
		auto functorize(R(C::*fn)(Params...) const) -> decltype(std::mem_fn(fn))
		{
			return std::mem_fn(fn);
		}
	}

	template <typename R, typename... Params>
	struct function_base
	{
#if 0
		auto operator()(Params... args) const -> R
		{
			return dispatch_(wrapper_.buf, args...);
		}

		template <typename... Args>
		auto operator ()(Args&&... args) const
			-> decltype(detail::dispatcher_t<R, std::tuple<Params...>, std::tuple<Args...>, sizeof...(Args)>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...))
		{
			return detail::dispatcher_t<R, std::tuple<Params...>, std::tuple<Args...>, sizeof...(Args)>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...);
		}
#endif

	protected:
#if 0
		detail::dispatch_fnptr_t<R, Params...>  dispatch_;
		detail::functor_wrapper_t<R, Params...> wrapper_;

		static auto empty_fn(Params...) -> R { return *reinterpret_cast<R*>(nullptr); }
#endif

	protected:
#if 0
		auto reset() -> void
		{
			dispatch_ = nullptr;
			wrapper_.~functor_wrapper_t();
		}

		auto init_empty() -> void
		{
			init_fn(&empty_fn);
		}

		template <typename FN>
		auto init_fn(FN&& fn) -> void
		{
			dispatch_ = &detail::vtable_impl_t<FN>::template call<R, Params...>;
			new (&wrapper_) detail::functor_wrapper_t<R, Params...>{std::forward<FN>(fn)};
		}
#endif
	};

	template <class T> struct function_base_getter;
	template <typename R, typename... Params> struct function_base_getter<R(Params...)>
	{
		typedef function_base<R, Params...> type;
	};

	template <class F>
	struct function : function_base_getter<F>::type
	{
#if 0
		function()
		{
			init_empty();
		}

		template <typename FN>
		explicit function(FN&& fn)
		{
			init_fn(detail::functorize(std::forward<FN>(fn)));
		}

		function(function const& rhs)
			: dispatch_{rhs.dispatch_}
			, wrapper_(rhs.wrapper_)
		{}

		function(function&& rhs)
			: dispatch_{rhs.dispatch_}
			, wrapper_{std::move(rhs.wrapper_)}
		{
		}

		auto operator = (function const& rhs) -> function&
		{
			if (this != &rhs)
			{
				reset();
				dispatch_ = rhs.dispatch_;
				wrapper_  = rhs.wrapper_;
			}

			return *this;
		}

		auto operator = (function&& rhs) -> function&
		{
			if (this != &rhs)
			{
				reset();
				dispatch_ = rhs.dispatch_;
				wrapper_  = std::move(rhs.wrapper_);
			}

			return *this;
		}

		template <typename FN>
		auto operator = (FN&& fn) -> function&
		{
			wrapper_.~functor_wrapper_t();
			init_fn(std::forward<FN>(fn));
			return *this;
		}

		auto operator = (std::nullptr_t) -> function&
		{
			wrapper_.~functor_wrapper_t();
			init_empty();
			return *this;
		}

		operator bool() const
		{
			return *target<R(*)(Params...)>() != empty_fn;
		}



		template <typename T>
		auto target() const -> T const*
		{
			return wrapper_.target<T>();
		}

		auto swap(function& rhs) -> void
		{
			auto tmp = detail::functor_wrapper_t{};
			rhs.wrapper_.move_into(tmp);
			wrapper_.move_into(rhs.wrapper_);
			tmp.move_into(wrapper_);
		}
#endif
	};


	namespace detail
	{
		template <typename R, typename... Params, typename Args, size_t I>
		template <typename... More>
		auto dispatcher_ii_t<R, std::tuple<Params...>, Args, I, false, true>::call(dispatch_fnptr_t<R, Params...> fn, functor_buf_t const& buf, Params... params, More&&...) -> R
		{
			return fn(buf, params...);
		};

		template <typename R, typename... Params, typename Args, size_t I>
		inline auto dispatcher_ii_t<R, std::tuple<Params...>, Args, I, false, false>::call(dispatch_fnptr_t<R, Params...> fn, functor_buf_t const& buf, Params... params) -> R
		{
			return fn(buf, params...);
		}

		template <typename R, typename... Params, typename... UParams, typename... RParams>
		auto dispatcher_iii_t<R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>::call(dispatch_fnptr_t<R, Params...> fn, functor_buf_t const& buf, UParams... params) -> function<R(RParams...)>
		{
			return function<R(RParams...)>{curry(fn, buf, params...)};
		}
	}
}
