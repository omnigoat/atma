#pragma once

#include <atma/tuple.hpp>
#include <tuple>

namespace atma
{
	template <typename R, typename... Params> struct function;




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

			destruct_fntype destruct;
			copy_fntype     copy;
			move_fntype     move;
		};

		template <typename FN, typename R, typename... Params>
		auto generate_vtable() -> functor_vtable_t<R, Params...>*
		{
			static auto _ = functor_vtable_t<R, Params...>
			{
				&vtable_impl_t<FN>::destruct,
					&vtable_impl_t<FN>::copy,
					&vtable_impl_t<FN>::move,
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
		template <bool, size_t, size_t, typename, typename>  struct dispatcher_t;
		template <typename, typename, typename>        struct dispatcher_partial_t;

		template <typename R, typename... Params>
		using dispatch_fnptr_t = auto(*)(functor_buf_t const&, Params...) -> R;

		template <typename R, typename... Params, typename... RParams>
		struct dispatcher_partial_t<R, std::tuple<Params...>, std::tuple<RParams...>>
		{
			static auto call(dispatch_fnptr_t<R, Params..., RParams...>, functor_buf_t const&, Params...) -> function<R(RParams...)>;
		};

		// too many args
#if 0
		template <size_t ParamCount, size_t ArgCount, typename R, typename... Params>
		struct dispatcher_t<false, ParamCount, ArgCount, R, std::tuple<Params...>>
			: dispatcher_t<ArgCount - 1 <= ParamCount, ParamCount, ArgCount - 1, R, std::tuple<Params...>>
		{
			using dispatcher_t<true, ParamCount, ArgCount - 1, R, std::tuple<Params...>>::call;

			static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Args...) -> R;
		};
#endif

		// completed call
		template <size_t ParamArgCount, typename R, typename... Params>
		struct dispatcher_t<true, ParamArgCount, ParamArgCount, R, std::tuple<Params...>>
			: dispatcher_t<true, ParamArgCount, ParamArgCount - 1, R, std::tuple<Params...>>
		{
			using dispatcher_t<true, ParamArgCount, ParamArgCount - 1, R, std::tuple<Params...>>::call;

			static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Params...) -> R;
		};

		// partial call
		template <size_t ParamCount, size_t ArgCount, typename R, typename... Params>
		struct dispatcher_t<true, ParamCount, ArgCount, R, std::tuple<Params...>>
			: dispatcher_t<true, ParamCount, ArgCount - 1, R, std::tuple<Params...>>
			, dispatcher_partial_t<R,
				atma::tuple_select_t<atma::idxs_list_t<ArgCount>, std::tuple<Params...>>,
				atma::tuple_select_t<atma::idxs_range_t<ArgCount, ParamCount>, std::tuple<Params...>>
			>
		{
			using dispatcher_partial_t<R,
				atma::tuple_select_t<atma::idxs_list_t<ArgCount>, std::tuple<Params...>>,
				atma::tuple_select_t<atma::idxs_range_t<ArgCount, ParamCount>, std::tuple<Params...>>
			>::call;
		};

		// terminate
		template <size_t ParamCount, typename R, typename... Params>
		struct dispatcher_t<true, ParamCount, 0, R, std::tuple<Params...>>
#if 0
			: dispatcher_partial_t<R,
			std::tuple<>,
			atma::tuple_select_t<atma::idxs_range_t<0, ParamCount>, std::tuple<Params...>>
			>
#endif
		{};

		namespace d2
		{
			template <typename R, typename Params, typename Args> struct dispatcher_t;

			template <typename R, typename Params, typename Args, typename Idxs>
			struct dispatcher_ii_t;

			template <typename R, typename Params, typename Args, size_t I, bool LT, bool GT> struct dispatcher_iv_t;

			template <typename R, typename... Params, typename Args, size_t I>
			struct dispatcher_iv_t<R, std::tuple<Params...>, Args, I, false, true>
			{
				template <typename... More>
				static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Params..., More&&...) -> R;
			};

			template <typename R, typename... Params, typename Args, size_t I>
			struct dispatcher_iv_t<R, std::tuple<Params...>, Args, I, false, false>
			{
				static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Params...) -> R;
			};

			template <typename R, typename Params, typename UParams, typename RParams> struct dispatcher_v_t;

			template <typename R, typename... Params, typename... UParams, typename... RParams>
			struct dispatcher_v_t<R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>
			{
				static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, UParams...) -> function<R(RParams...)>;
			};

			template <typename R, typename... Params, typename Args, size_t I>
			struct dispatcher_iv_t<R, std::tuple<Params...>, Args, I, true, false>
				: dispatcher_v_t<R, std::tuple<Params...>,
					tuple_select_t<idxs_list_t<I>, std::tuple<Params...>>,
					tuple_select_t<idxs_range_t<I, sizeof...(Params)>, std::tuple<Params...>>>
			{
			};


			template <typename R, typename Params, typename Args, size_t I>
			struct dispatcher_iii_t;

			template <typename R, typename... Params, typename Args, size_t I>
			struct dispatcher_iii_t<R, std::tuple<Params...>, Args, I>
				: dispatcher_iv_t<R, std::tuple<Params...>, Args, I, (I < sizeof...(Params)), (sizeof...(Params) < I)>
			{
			};

			template <typename R, typename... Params, typename... Args, size_t... idxs>
			struct dispatcher_ii_t<R, std::tuple<Params...>, std::tuple<Args...>, idxs_t<idxs...>>
				: dispatcher_iii_t<R, std::tuple<Params...>, std::tuple<Args...>, idxs>...
			{
			};


			template <typename R, typename... Params, typename... Args>
			struct dispatcher_t<R, std::tuple<Params...>, std::tuple<Args...>>
				: dispatcher_ii_t<R, std::tuple<Params...>, std::tuple<Args...>, idxs_list_t<sizeof...(Args) + 1>>
			{
			};
		}
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
	struct function<R(Params...)>
	{
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

		auto operator()(Params... args) const -> R
		{
			return dispatch_(wrapper_.buf, args...);
		}

		template <typename... Args>
		auto operator ()(Args&&... args) const
			-> decltype(
				//detail::dispatcher_t<sizeof...(Args) <= sizeof...(Params), sizeof...(Params), sizeof...(Args), R, std::tuple<Params...>>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...)
				detail::d2::dispatcher_iii_t<R, std::tuple<Params...>, std::tuple<Args...>, sizeof...(Args)>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...)
			)
		{
			//return detail::dispatcher_t<sizeof...(Args) <= sizeof...(Params), sizeof...(Params), sizeof...(Args), R, std::tuple<Params...>>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...);
			//return detail::d2::dispatcher_t<R, std::tuple<Params...>, std::tuple<Args...>>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...);
			return detail::d2::dispatcher_iii_t<R, std::tuple<Params...>, std::tuple<Args...>, sizeof...(Args)>::call(dispatch_, wrapper_.buf, std::forward<Args>(args)...);
		}

		auto swap(function& rhs) -> void
		{
			auto tmp = detail::functor_wrapper_t{};
			rhs.wrapper_.move_into(tmp);
			wrapper_.move_into(rhs.wrapper_);
			tmp.move_into(wrapper_);
		}

	private:
		auto reset() -> void
		{
			dispatch_ = nullptr;
			wrapper_.~functor_wrapper_t();
		}

		auto init_empty() -> void
		{
			using empty_fn = auto(*)(Params...) -> R;
			init_fn(empty_fn());
		}

		template <typename FN>
		auto init_fn(FN&& fn) -> void
		{
			dispatch_ = &detail::vtable_impl_t<FN>::template call<R, Params...>;
			new (&wrapper_) detail::functor_wrapper_t<R, Params...>{std::forward<FN>(fn)};
		}

	public:
		detail::dispatch_fnptr_t<R, Params...>  dispatch_;
		detail::functor_wrapper_t<R, Params...> wrapper_;
	};


	namespace detail
	{
		namespace d2
		{
			template <typename R, typename... Params, typename Args, size_t I>
			template <typename... More>
			auto dispatcher_iv_t<R, std::tuple<Params...>, Args, I, false, true>::call(dispatch_fnptr_t<R, Params...> fn, functor_buf_t const& buf, Params... params, More&&...) -> R
			{
				return fn(buf, params...);
			};

			template <typename R, typename... Params, typename Args, size_t I>
			inline auto dispatcher_iv_t<R, std::tuple<Params...>, Args, I, false, false>::call(dispatch_fnptr_t<R, Params...> fn, functor_buf_t const& buf, Params... params) -> R
			{
				return fn(buf, params...);
			}

			template <typename R, typename... Params, typename... UParams, typename... RParams>
			auto dispatcher_v_t<R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>::call(dispatch_fnptr_t<R, Params...> fn, functor_buf_t const& buf, UParams... params) -> function<R(RParams...)>
			{
				return function<R(RParams...)>{curry(fn, buf, params...)};
			}
		}

		template <typename R, typename... Params, typename... RParams>
		inline auto dispatcher_partial_t<R, std::tuple<Params...>, std::tuple<RParams...>>
			::call(dispatch_fnptr_t<R, Params..., RParams...> dispatch, functor_buf_t const& buf, Params... args)
			-> function<R(RParams...)>
		{
			return function<R(RParams...)>{atma::curry(dispatch, buf, args...)};
		}

		template <size_t S, typename R, typename... Params>
		inline auto dispatcher_t<true, S, S, R, std::tuple<Params...>>::call(dispatch_fnptr_t<R, Params...> dispatch, functor_buf_t const& buf, Params... args)
			-> R
		{
			return dispatch(buf, args...);
		}
	}
}
